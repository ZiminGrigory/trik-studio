/* Copyright 2017 CyberTech Labs Ltd.
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

#include "calibrateGyroscopeGenerator.h"

#include <generatorBase/generatorCustomizer.h>

#include "ev3GeneratorBase/ev3GeneratorFactory.h"

using namespace ev3::simple;
using namespace generatorBase::simple;
using namespace qReal;

CalibrateGyroscopeGenerator::CalibrateGyroscopeGenerator(const qrRepo::RepoApi &repo
		, generatorBase::GeneratorCustomizer &customizer
		, const Id &id
		, QObject *parent)
	: BindingGenerator(repo, customizer, id, "sensors/calibrateGyroscope.t", QList<Binding *>()
			<< Binding::createConverting("@@PORT@@", "Port"
					, static_cast<Ev3GeneratorFactory *>(customizer.factory())->portNameConverter())
			, parent)
{
}
