/** @file editorviewscene.cpp
 * 	@brief Сцена для отрисовки объектов
 * */
#include "editorviewscene.h"

#include <QGraphicsTextItem>
#include <QtGui>

#include "editorviewmviface.h"
#include "editorview.h"
#include "mainwindow.h"
#include "../mainwindow/mainwindow.h"
#include "../model/model.h"

using namespace qReal;

EditorViewScene::EditorViewScene(QObject * parent)
		:  QGraphicsScene(parent), mWindow(NULL), mPrevParent(0)
{
	setItemIndexMethod(NoIndex);
	setEnabled(false);
}

void EditorViewScene::setEnabled(bool enabled)
{
	foreach (QGraphicsView *view, views())
		view->setEnabled(enabled);
}

void EditorViewScene::clearScene()
{
	foreach (QGraphicsItem *item, items())
		// Выглядит довольно безумно, но некоторые элементы
		// оказываются уже удалены, потому как был удалён их родитель.
		if (items().contains(item))
			removeItem(item);
}

UML::Element * EditorViewScene::getElem(qReal::Id const &uuid)
{
	if (uuid == ROOT_ID)
		return NULL;

	// FIXME: SLOW!
	QList < QGraphicsItem * > list = items();
	for (QList < QGraphicsItem * >::Iterator it = list.begin(); it != list.end(); ++it) {
		if (UML::Element * elem = dynamic_cast < UML::Element * >(*it)) {
			if (elem->uuid() == uuid) {
				return elem;
			}
		}
	}

	return NULL;
}

UML::Element * EditorViewScene::getElemByModelIndex(const QModelIndex &ind)
{
	// FIXME: SLOW!
	QList < QGraphicsItem * > list = items();
	for (QList < QGraphicsItem * >::Iterator it = list.begin(); it != list.end(); ++it) {
		if (UML::Element * elem = dynamic_cast < UML::Element * >(*it)) {
			if (elem->index() == ind)
				return elem;
		}
	}
	Q_ASSERT(!"Element not found");
	return NULL;
}

void EditorViewScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();
	if (mimeData->hasFormat("application/x-real-uml-data"))
		QGraphicsScene::dragEnterEvent(event);
	else
		event->ignore();
}

void EditorViewScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
	Q_UNUSED(event);
}

void EditorViewScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
	Q_UNUSED(event);
}

bool EditorViewScene::canBeContainedBy(qReal::Id container, qReal::Id candidate)
{
	bool allowed = false;
	foreach (qReal::Id type, mWindow->manager()->getContainedTypes(container)){
		if (candidate.element() ==  type.editor() )
			allowed = true;
	}	
	return allowed;
}

void EditorViewScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
	Q_ASSERT(mWindow);  // Значение mWindow должно быть инициализировано
	// отдельно, через конструктор это делать нехорошо,
	// поскольку сцена создаётся в сгенерённом ui-шнике.

	// если нет ни одной диаграммы, то ничего не создаем.
	if (mv_iface->model()->rowCount(QModelIndex()) == 0)
		return;

	// Transform mime data to include coordinates.
	const QMimeData *mimeData = event->mimeData();
	QByteArray itemData = mimeData->data("application/x-real-uml-data");

	QDataStream in_stream(&itemData, QIODevice::ReadOnly);

	QString uuid = "";
	QString pathToItem = "";
	QString name;
	QPointF pos;

	in_stream >> uuid;
	in_stream >> pathToItem;
	in_stream >> name;
	in_stream >> pos;

	QByteArray newItemData;
	QDataStream stream(&newItemData, QIODevice::WriteOnly);

	UML::Element *newParent = NULL;

	// TODO: возможно, это можно сделать проще
	qReal::Id id = qReal::Id::loadFromString(uuid);
	UML::Element *e = mWindow->manager()->graphicalObject(id);
	//	= UML::GUIObjectFactory(type_id);

	if (dynamic_cast<UML::NodeElement*>(e))
		newParent = getElemAt(event->scenePos());

	if (e)
		delete e;

	if( newParent ){
		if (!canBeContainedBy(newParent->uuid(), id)){
			QMessageBox::critical(0, "Error!", "[some text]");
			return;
		}
	}	
 
	stream << uuid;				// uuid
	stream << pathToItem;
	stream << name;

	if (!newParent)
		stream << event->scenePos();
	else
		stream << newParent->mapToItem(newParent, newParent->mapFromScene(event->scenePos()));

	QMimeData *newMimeData = new QMimeData;
	newMimeData->setData("application/x-real-uml-data", newItemData);

	QModelIndex parentIndex = newParent ? QModelIndex(newParent->index()) : mv_iface->rootIndex();

	mv_iface->model()->dropMimeData(newMimeData, event->dropAction(),
		mv_iface->model()->rowCount(parentIndex), 0, parentIndex);

	delete newMimeData;
}

void EditorViewScene::keyPressEvent(QKeyEvent *event)
{
	if (dynamic_cast<QGraphicsTextItem*>(this->focusItem())) {
		// Forward event to text editor
		QGraphicsScene::keyPressEvent(event);
	} else if (event->key() == Qt::Key_Delete) {
		// Delete selected elements from scene
		mainWindow()->deleteFromScene();
	} else
		QGraphicsScene::keyPressEvent(event);
}

void EditorViewScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	// Let scene update selection and perform other operations
	QGraphicsScene::mousePressEvent(event);

	if( event->button() == Qt::LeftButton ){
		QGraphicsItem *item = itemAt(event->scenePos());
		qDebug() << "item: " << item;
		UML::ElementTitle *title = dynamic_cast < UML::ElementTitle * >(item);
		if( title ){ // проверяем, а не зацепились ли мы случайно за надпись, когда начали тащить эоемент
			item = item->parentItem();
		}	
		if( item ){			
			mPrevParent = item->parentItem();
			mPrevPosition = item->scenePos();	
			qDebug() << "NEW mPrevParent: " << mPrevParent;
			qDebug() << "NEW pos: " << mPrevPosition;
		}	

	} else if (event->button() == Qt::RightButton){

		UML::Element *e = getElemAt(event->scenePos());
		if (!e)
			return;
		if (!e->isSelected()) {
			clearSelection();
			e->setSelected(true);
		}

		// Menu belongs to scene handler because it can delete elements.
		// We cannot not allow elements to commit suicide.
	
		QMenu menu;
		menu.addAction(mWindow->ui.actionDeleteFromDiagram);
		QList<UML::ContextMenuAction*> elementActions = e->contextMenuActions();
	
		if (!elementActions.isEmpty())
			menu.addSeparator();

		foreach (UML::ContextMenuAction* action, elementActions) {
			action->setEventPos(e->mapFromScene(event->scenePos()));
			menu.addAction(action);
		}

		// FIXME: add check for diagram
		//	if (selectedItems().count() == 1)
		//		menu.addAction(window->ui.actionJumpToAvatar);

		menu.exec(QCursor::pos());
	}	
}

void EditorViewScene::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
	// Let scene update selection and perform other operations
	QGraphicsScene::mouseReleaseEvent(event);
	
	UML::Element *element = getElemAt(event->scenePos());
	if (!element)
		return;
	
	qDebug() << "element" << element->uuid().toString();
	UML::Element *parent = getElemByModelIndex(element->index().parent());
					
	
	if (parent){
	qDebug() << "parent: " << parent->uuid().toString();
	
		if (!canBeContainedBy(parent->uuid(), element->uuid())){
			QMessageBox::critical(0, "Ololo", "can't drop it here!111");
			// фэйл, репарентим элемент обратно
			foreach (QGraphicsItem *item, items(event->scenePos())) {
				UML::Element * elem = dynamic_cast < UML::Element * >(item);
				if (elem && elem->uuid() == element->uuid()) {
					QModelIndex ind = mv_iface->rootIndex();
					UML::Element * prevParent = dynamic_cast < UML::Element * >(mPrevParent);
					qDebug() << "prev parent:  " << mPrevParent;
					
					if (prevParent)
						ind = prevParent->index();
					
					qReal::model::Model *model = dynamic_cast < qReal::model::Model *> (mv_iface->model());
					if (model)
						model->changeParent(element->index(), ind, mPrevPosition);		
					
//					elem->setParentItem(mPrevParent);
//					elem->setPos(mPrevPosition);
					qDebug() << "new pos: " << elem->scenePos() << elem->pos();
				}
			}
		}
	}
}

void EditorViewScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		// Double click on a title activates it
		if (UML::ElementTitle *title = dynamic_cast<UML::ElementTitle*>(itemAt(event->scenePos()))) {
			if (!title->hasFocus()) {  // Do not activate already activated item
				event->accept();
				title->startTextInteraction();
				return;
			}
		}
		else if (UML::NodeElement *element = dynamic_cast<UML::NodeElement*>(itemAt(event->scenePos()))) {
			event->accept();
			mainWindow()->activateSubdiagram(element->index());
			// Now scene is changed from outside. Being a mere mortal I do not
			// know whether it is good or not, but what is the destiny of
			// workflow after this return?
			return;
		}
	}

	QGraphicsScene::mouseDoubleClickEvent(event);
}

UML::Element* EditorViewScene::getElemAt(QPointF const &position)
{
	foreach (QGraphicsItem *item, items(position)) {
		UML::Element *e = dynamic_cast<UML::Element *>(item);
		if (e)
			return e;
	}
	return NULL;
}

QPersistentModelIndex EditorViewScene::rootItem()
{
	return mv_iface->rootIndex();
}

void EditorViewScene::setMainWindow(qReal::MainWindow *mainWindow)
{
	mWindow = mainWindow;
}

qReal::MainWindow *EditorViewScene::mainWindow() const
{
	return mWindow;
}
