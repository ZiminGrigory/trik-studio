#include "blocksTable.h"

using namespace qReal;
using namespace interpreterBase::blocksBase;
using namespace interpreterCore::interpreter::details;

BlocksTable::BlocksTable(BlocksFactoryManagerInterface &blocksFactoryManager)
	: mBlocksFactoryManager(blocksFactoryManager)
{
}

BlocksTable::~BlocksTable()
{
	qDeleteAll(mBlocks);
}

BlockInterface *BlocksTable::block(Id const &element)
{
	if (mBlocks.contains(element)) {
		return mBlocks[element];
	}

	BlockInterface *newBlock = mBlocksFactoryManager.block(element);
	mBlocks.insert(element, newBlock);
	return newBlock;
}

void BlocksTable::clear()
{
	qDeleteAll(mBlocks);
	mBlocks.clear();
}

void BlocksTable::setFailure()
{
	for (BlockInterface * const block : mBlocks.values()) {
		block->setFailedStatus();
	}
}

void BlocksTable::setIdleForBlocks()
{
	for (BlockInterface * const block : mBlocks.values()) {
		block->setIdleStatus();
	}
}

qReal::IdList BlocksTable::providedBlocks() const
{
	return mBlocksFactoryManager.providedBlocks();
}
