/* Copyright 2015 CyberTech Labs Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#include "ev3LuaPrinter.h"

#include <qrkernel/logging.h>

#include <qrtext/lua/ast/number.h>
#include <qrtext/lua/ast/unaryMinus.h>
#include <qrtext/lua/ast/not.h>
#include <qrtext/lua/ast/bitwiseNegation.h>
#include <qrtext/lua/ast/length.h>
#include <qrtext/lua/ast/logicalAnd.h>
#include <qrtext/lua/ast/logicalOr.h>
#include <qrtext/lua/ast/addition.h>
#include <qrtext/lua/ast/subtraction.h>
#include <qrtext/lua/ast/multiplication.h>
#include <qrtext/lua/ast/division.h>
#include <qrtext/lua/ast/integerDivision.h>
#include <qrtext/lua/ast/modulo.h>
#include <qrtext/lua/ast/exponentiation.h>
#include <qrtext/lua/ast/bitwiseAnd.h>
#include <qrtext/lua/ast/bitwiseOr.h>
#include <qrtext/lua/ast/bitwiseXor.h>
#include <qrtext/lua/ast/bitwiseLeftShift.h>
#include <qrtext/lua/ast/bitwiseRightShift.h>
#include <qrtext/lua/ast/concatenation.h>
#include <qrtext/lua/ast/equality.h>
#include <qrtext/lua/ast/lessThan.h>
#include <qrtext/lua/ast/lessOrEqual.h>
#include <qrtext/lua/ast/inequality.h>
#include <qrtext/lua/ast/greaterThan.h>
#include <qrtext/lua/ast/greaterOrEqual.h>
#include <qrtext/lua/ast/integerNumber.h>
#include <qrtext/lua/ast/floatNumber.h>
#include <qrtext/lua/ast/fieldInitialization.h>
#include <qrtext/lua/ast/tableConstructor.h>
#include <qrtext/lua/ast/string.h>
#include <qrtext/lua/ast/true.h>
#include <qrtext/lua/ast/false.h>
#include <qrtext/lua/ast/nil.h>
#include <qrtext/lua/ast/identifier.h>
#include <qrtext/lua/ast/functionCall.h>
#include <qrtext/lua/ast/methodCall.h>
#include <qrtext/lua/ast/assignment.h>
#include <qrtext/lua/ast/block.h>
#include <qrtext/lua/ast/indexingExpression.h>
#include <qrtext/languageToolboxInterface.h>
#include <qrtext/lua/types/integer.h>
#include <qrtext/lua/types/float.h>
#include <qrtext/lua/types/boolean.h>
#include <qrtext/lua/types/string.h>

using namespace ev3::rbf::lua;

const QMap<Ev3RbfType, QString> registerNames = {
	{ Ev3RbfType::data8, "_bool_temp_result_" }
	, { Ev3RbfType::data16, "_small_int_temp_result_" }
	, { Ev3RbfType::data32, "_int_temp_result_" }
	, { Ev3RbfType::dataF, "_float_temp_result_" }
	, { Ev3RbfType::dataS, "_string_temp_result_" }
};

const QMap<Ev3RbfType, QString> typeNames = {
	{ Ev3RbfType::data8, "8" }
	, { Ev3RbfType::data16, "16" }
	, { Ev3RbfType::data32, "32" }
	, { Ev3RbfType::dataF, "F" }
	, { Ev3RbfType::dataS, "S" }
};

Ev3LuaPrinter::Ev3LuaPrinter(const QStringList &pathsToTemplates
		, const qrtext::LanguageToolboxInterface &textLanguage
		, generatorBase::parts::Variables &variables
		, const generatorBase::simple::Binding::ConverterInterface *reservedVariablesConverter)
	: generatorBase::TemplateParametrizedEntity(addSuffix(pathsToTemplates))
	, mTextLanguage(textLanguage)
	, mVariables(variables)
	, mReservedVariablesConverter(reservedVariablesConverter)
	, mReservedFunctionsConverter(pathsToTemplates)
{
}

Ev3LuaPrinter::~Ev3LuaPrinter()
{
	delete mReservedVariablesConverter;
}

QStringList Ev3LuaPrinter::addSuffix(const QStringList &list)
{
	QStringList result;
	for (const QString &path : list) {
		result << path + "/luaPrinting";
	}

	return result;
}

Ev3RbfType Ev3LuaPrinter::toEv3Type(const QSharedPointer<qrtext::core::types::TypeExpression> &type)
{
	if (type->is<qrtext::lua::types::Boolean>()) {
		return Ev3RbfType::data8;
	}

	if (type->is<qrtext::lua::types::Integer>()) {
		return Ev3RbfType::data32;
	}

	if (type->is<qrtext::lua::types::Float>()) {
		return Ev3RbfType::dataF;
	}

	if (type->is<qrtext::lua::types::String>()) {
		return Ev3RbfType::dataS;
	}

	qWarning() << "Ev3LuaPrinter::typeOf: Unsupported type" << type->toString();
	return Ev3RbfType::other;
}

Ev3RbfType Ev3LuaPrinter::typeOf(const QSharedPointer<qrtext::lua::ast::Node> &node)
{
	return toEv3Type(mTextLanguage.type(node));
}

QString Ev3LuaPrinter::newRegister(const QSharedPointer<qrtext::lua::ast::Node> &node)
{
	return newRegister(typeOf(node));
}

QString Ev3LuaPrinter::newRegister(Ev3RbfType type)
{
	if (type == Ev3RbfType::other) {
		return QString();
	}

	const QString result = registerNames[type] + QString::number(++mRegistersCount[mId][type]);
	mVariables.appendManualDeclaration(QString("DATA%1 %2").arg(typeNames[type], result));
	return result;
}

QString Ev3LuaPrinter::print(const QSharedPointer<qrtext::lua::ast::Node> &node, const qReal::Id &id)
{
	mId = id;
	return printWithoutPop(node) ? popResult(node) : QString();
}

QString Ev3LuaPrinter::castTo(const QSharedPointer<qrtext::core::types::TypeExpression> &type
		, const QSharedPointer<qrtext::lua::ast::Node> &node, const qReal::Id &id)
{
	mId = id;
	return printWithoutPop(node) ? castTo(toEv3Type(type), node) : QString();
}

QStringList Ev3LuaPrinter::additionalCode(const qReal::Id &id) const
{
	return mAdditionalCode[id];
}

void Ev3LuaPrinter::pushResult(const QSharedPointer<qrtext::lua::ast::Node> &node
		, const QString &generatedCode, const QString &additionalCode)
{
	mGeneratedCode[node.data()] = generatedCode;
	if (!additionalCode.isEmpty()) {
		mAdditionalCode[mId] << additionalCode;
	}
}

QString Ev3LuaPrinter::popResult(const QSharedPointer<qrtext::lua::ast::Node> &node)
{
	return mGeneratedCode.take(node.data());
}

QStringList Ev3LuaPrinter::popResults(const QList<QSharedPointer<qrtext::lua::ast::Node>> &nodes)
{
	QStringList result;
	for (const QSharedPointer<qrtext::lua::ast::Node> &node : nodes) {
		result << popResult(node);
	}

	return result;
}

bool Ev3LuaPrinter::printWithoutPop(const QSharedPointer<qrtext::lua::ast::Node> &node)
{
	if (!node) {
		return false;
	}

	node->acceptRecursively(*this, node);
	if (mGeneratedCode.keys().count() != 1 || mGeneratedCode.keys().first() != node.data()) {
		QLOG_WARN() << "Lua printer got into the inconsistent state during printing."
				<< mGeneratedCode.keys().count() << "pieces of code:";
		for (const QString &code : mGeneratedCode.values()) {
			QLOG_INFO() << code;
		}

		mGeneratedCode.clear();
		return false;
	}

	return true;
}

void Ev3LuaPrinter::processTemplate(const QSharedPointer<qrtext::lua::ast::Node> &node
		, const QString &templateFileName
		, QMap<QString, QSharedPointer<qrtext::lua::ast::Node>> const &bindings
		, const QMap<QString, QString> &staticBindings)
{
	QString computation = readTemplate(templateFileName);
	const bool typedComputation = computation.contains("@@RESULT@@");
	QString result;
	if (typedComputation) {
		result = newRegister(node);
		computation.replace("@@RESULT@@", result);
	}

	for (const QString &toReplace : bindings.keys()) {
		computation.replace(toReplace, popResult(bindings[toReplace]));
	}

	for (const QString &toReplace : staticBindings.keys()) {
		computation.replace(toReplace, staticBindings[toReplace]);
	}

	pushResult(node, typedComputation ? result : computation, typedComputation ? computation : QString());
}

void Ev3LuaPrinter::processUnary(const QSharedPointer<qrtext::core::ast::UnaryOperator> &node
		, const QString &templateFileName)
{
	const Ev3RbfType type = typeOf(node);
	const QString result = newRegister(type);
	pushResult(node, result, readTemplate(templateFileName)
			.replace("@@TYPE@@", typeNames[type])
			.replace("@@RESULT@@", result)
			.replace("@@OPERAND@@", popResult(node->operand())));
}

void Ev3LuaPrinter::processBinary(const QSharedPointer<qrtext::core::ast::BinaryOperator> &node
		, Ev3RbfType operandsType, Ev3RbfType resultType, const QString &templateFileName)
{
	const QString result = newRegister(resultType);
	pushResult(node, result, readTemplate(templateFileName)
			.replace("@@TYPE@@", typeNames[operandsType])
			.replace("@@RESULT@@", result)
			.replace("@@LEFT@@", castTo(operandsType, node->leftOperand()))
			.replace("@@RIGHT@@", castTo(operandsType, node->rightOperand())));
}

void Ev3LuaPrinter::processBinary(const QSharedPointer<qrtext::core::ast::BinaryOperator> &node
		, Ev3RbfType resultType, const QString &templateFileName)
{
	const Ev3RbfType operandsType = moreCommon(typeOf(node->leftOperand()), typeOf(node->rightOperand()));
	processBinary(node, operandsType, resultType, templateFileName);
}

void Ev3LuaPrinter::processBinary(const QSharedPointer<qrtext::core::ast::BinaryOperator> &node
		, const QString &templateFileName)
{
	const Ev3RbfType type = moreCommon(typeOf(node->leftOperand()), typeOf(node->rightOperand()));
	processBinary(node, type, type, templateFileName);
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Number> &node)
{
	pushResult(node, node->stringRepresentation(), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::UnaryMinus> &node)
{
	processUnary(node, "unaryMinus.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Not> &node)
{
	processUnary(node, "not.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::BitwiseNegation> &node)
{
	processUnary(node, "bitwiseNegation.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Length> &node)
{
	processUnary(node, "length.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::LogicalAnd> &node)
{
	processBinary(node, Ev3RbfType::data8, Ev3RbfType::data8, "logicalAnd.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::LogicalOr> &node)
{
	processBinary(node, Ev3RbfType::data8, Ev3RbfType::data8, "logicalOr.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Addition> &node)
{
	processBinary(node, "addition.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Subtraction> &node)
{
	processBinary(node, "subtraction.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Multiplication> &node)
{
	processBinary(node, "multiplication.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Division> &node)
{
	processBinary(node, Ev3RbfType::dataF, Ev3RbfType::dataF, "division.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::IntegerDivision> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "integerDivision.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Modulo> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "modulo.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Exponentiation> &node)
{
	processBinary(node, Ev3RbfType::dataF, Ev3RbfType::dataF, "exponentiation.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::BitwiseAnd> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "bitwiseAnd.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::BitwiseOr> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "bitwiseOr.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::BitwiseXor> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "bitwiseXor.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::BitwiseLeftShift> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "bitwiseLeftShift.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::BitwiseRightShift> &node)
{
	processBinary(node, Ev3RbfType::data32, Ev3RbfType::data32, "bitwiseRightShift.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Concatenation> &node)
{
	const QString result = newRegister(node);
	pushResult(node, result, readTemplate("concatenation.t")
			.replace("@@RESULT@@", result)
			.replace("@@LEFT@@", toString(node->leftOperand()))
			.replace("@@RIGHT@@", toString(node->rightOperand())));
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Equality> &node)
{
	processBinary(node, Ev3RbfType::data8, "equality.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::LessThan> &node)
{
	processBinary(node, Ev3RbfType::data8, "lessThan.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::LessOrEqual> &node)
{
	processBinary(node, Ev3RbfType::data8, "lessOrEqual.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Inequality> &node)
{
	processBinary(node, Ev3RbfType::data8, "inequality.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::GreaterThan> &node)
{
	processBinary(node, Ev3RbfType::data8, "greaterThan.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::GreaterOrEqual> &node)
{
	processBinary(node, Ev3RbfType::data8, "greaterOrEqual.t");
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::IntegerNumber> &node)
{
	pushResult(node, node->stringRepresentation(), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::FloatNumber> &node)
{
	pushResult(node, node->stringRepresentation() + "F", QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::FieldInitialization> &node)
{
	const QString templatePath = node->key().data()
			? "explicitKeyFieldInitialization.t"
			: "implicitKeyFieldInitialization.t";
	processTemplate(node, templatePath, { {"@@KEY@@", node->key()}, {"@@VALUE@@", node->value()} });
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::TableConstructor> &node)
{
	const QStringList initializers = popResults(qrtext::as<qrtext::lua::ast::Node>(node->initializers()));
	pushResult(node, readTemplate("tableConstructor.t")
			.replace("@@COUNT@@", QString::number(initializers.count()))
			.replace("@@INITIALIZERS@@", initializers.join(readTemplate("fieldInitializersSeparator.t"))), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::String> &node)
{
	pushResult(node, readTemplate("string.t").replace("@@VALUE@@", node->string()), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::True> &node)
{
	pushResult(node, readTemplate("true.t"), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::False> &node)
{
	pushResult(node, readTemplate("false.t"), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Nil> &node)
{
	pushResult(node, readTemplate("nil.t"), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Identifier> &node)
{
	QString additionalCode;
	QString result = mReservedVariablesConverter->convert(node->name());
	if (result != node->name()) {
		const QString registerName = newRegister(node);
		additionalCode = result.replace("@@RESULT@@", registerName);
		result = registerName;
	}

	pushResult(node, result, additionalCode);
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::FunctionCall> &node)
{
	const QString expression = popResult(node->function());
	const QStringList arguments = popResults(qrtext::as<qrtext::lua::ast::Node>(node->arguments()));

	const qrtext::lua::ast::Identifier *idNode = dynamic_cast<qrtext::lua::ast::Identifier *>(node->function().data());
	const QString reservedFunctionCall = idNode
			? mReservedFunctionsConverter.convert(idNode->name(), arguments)
			: QString();

	if (reservedFunctionCall.isEmpty()) {
		pushResult(node, readTemplate("functionCall.t")
				.replace("@@FUNCTION@@", expression)
				.replace("@@ARGUMENTS@@", arguments.join(readTemplate("argumentsSeparator.t"))), QString());
	} else {
		pushResult(node, reservedFunctionCall, QString());
	}
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::MethodCall> &node)
{
	const QString object = popResult(node->object());
	const QString method = popResult(node->methodName());
	const QStringList arguments = popResults(qrtext::as<qrtext::lua::ast::Node>(node->arguments()));
	pushResult(node, readTemplate("methodCall.t")
			.replace("@@OBJECT@@", object)
			.replace("@@METHOD@@", method)
			.replace("@@ARGUMENTS@@", arguments.join(readTemplate("argumentsSeparator.t"))), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Assignment> &node)
{
	processTemplate(node, "assignment.t", { {"@@VARIABLE@@", node->variable()}, {"@@VALUE@@", node->value()} }
			, { {"@@TYPE1@@", typeNames[typeOf(node->variable())]} , {"@@TYPE2@@", typeNames[typeOf(node->value())]} }
	);
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::Block> &node)
{
	const QStringList expressions = popResults(node->children());
	pushResult(node, expressions.join(readTemplate("statementsSeparator.t")), QString());
}

void Ev3LuaPrinter::visit(const QSharedPointer<qrtext::lua::ast::IndexingExpression> &node)
{
	processTemplate(node, "indexingExpression.t", { {"@@TABLE@@", node->table()}, {"@@INDEXER@@", node->indexer()} });
}

QString Ev3LuaPrinter::toString(const QSharedPointer<qrtext::lua::ast::Node> &node)
{
	const QSharedPointer<qrtext::core::types::TypeExpression> type = mTextLanguage.type(node);
	const QString value = popResult(node);
	if (type->is<qrtext::lua::types::String>()) {
		return value;
	}

	if (type->is<qrtext::lua::types::Integer>()) {
		return readTemplate("intToString.t").replace("@@VALUE@@", value);
	}

	if (type->is<qrtext::lua::types::Float>()) {
		return readTemplate("floatToString.t").replace("@@VALUE@@", value);
	}

	if (type->is<qrtext::lua::types::Integer>()) {
		return readTemplate("boolToString.t").replace("@@VALUE@@", value);
	}

	return readTemplate("otherToString.t").replace("@@VALUE@@", value);
}

QString Ev3LuaPrinter::castTo(Ev3RbfType targetType, const QSharedPointer<qrtext::lua::ast::Node> &node)
{
	const Ev3RbfType actualType = typeOf(node);
	if (targetType == Ev3RbfType::dataS) {
		return toString(node);
	}

	const QString value = popResult(node);
	if (actualType == targetType) {
		return value;
	}

	if (actualType == Ev3RbfType::dataS) {
		return QObject::tr("/* Warning: cast from string to int is not supported */ 0");
	}

	if (targetType == Ev3RbfType::other || actualType == Ev3RbfType::other) {
		return QObject::tr("/* Warning: autocast is supported only for numeric types */ 0");
	}

	const QString result = newRegister(targetType);
	mAdditionalCode[mId] << QString("MOVE%1_%2(%3, %4)")
			.arg(typeNames[actualType], typeNames[targetType], value, result);
	return result;
}

Ev3RbfType Ev3LuaPrinter::moreCommon(Ev3RbfType type1, Ev3RbfType type2) const
{
	return static_cast<Ev3RbfType>(qMax(static_cast<Ev3RbfType>(type1), static_cast<Ev3RbfType>(type2)));
}
