/* Copyright 2017 QReal Research Group
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

#pragma once

#include <QtCore/QScopedPointer>

#include <generatorBase/robotsGeneratorPluginBase.h>

namespace pioneer {

namespace blocks {
class PioneerBlocksFactory;
}

namespace lua {

class PioneerGeneratorRobotModel;
class PioneerAdditionalPreferences;

/// Main class for Pioneer Lua generator plugin.
class PioneerLuaGeneratorPlugin : public generatorBase::RobotsGeneratorPluginBase
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "pioneer.lua.PioneerLuaGeneratorPlugin")

public:
	PioneerLuaGeneratorPlugin();
	~PioneerLuaGeneratorPlugin() override;

	QList<qReal::ActionInfo> customActions() override;

	QList<qReal::HotKeyActionInfo> hotKeyActions() override;

	QIcon iconForFastSelector(const kitBase::robotModel::RobotModelInterface &robotModel) const override;

	int priority() const override;

	QString kitId() const override;

	QList<kitBase::robotModel::RobotModelInterface *> robotModels() override;

	kitBase::blocksBase::BlocksFactoryInterface *blocksFactoryFor(
			const kitBase::robotModel::RobotModelInterface *model
			) override;

	QList<kitBase::AdditionalPreferences *> settingsWidgets() override;

private slots:
	/// Uploads current program to a quadcopter.
	void uploadProgram();

	/// Attempts to run current program on a quadcopter. Generates and uploads it first, if needed.
	void runProgram();

private:
	generatorBase::MasterGeneratorBase *masterGenerator() override;

	QString defaultFilePath(const QString &projectName) const override;

	qReal::text::LanguageInfo language() const override;

	QString generatorName() const override;

	void regenerateExtraFiles(const QFileInfo &newFileInfo) override;

private:
	/// Action that launches code generator.
	QAction *mGenerateCodeAction;  // Doesn't have ownership; may be disposed by GUI.

	/// Action that uploads generated program onto quadcopter.
	QAction *mUploadProgramAction;  // Doesn't have ownership; may be disposed by GUI.

	/// Action that executes program on a quadcopter.
	QAction *mRunProgramAction;  // Doesn't have ownership; may be disposed by GUI.

	/// Factory for blocks on a diagram that can be processed by this generator.
	blocks::PioneerBlocksFactory *mBlocksFactory;  // Transfers ownership

	/// Robot model for a generator, defines robot name and other unneeded properties.
	QScopedPointer<PioneerGeneratorRobotModel> mGeneratorRobotModel;

	PioneerAdditionalPreferences *mAdditionalPreferences = nullptr;  // Transfers ownership
	bool mOwnsAdditionalPreferences = true;
};

}
}
