#include "drawRectGenerator.h"
#include <generatorBase/generatorCustomizer.h>

using namespace ev3::simple;
using namespace generatorBase::simple;

DrawRectGenerator::DrawRectGenerator(qrRepo::RepoApi const &repo
		, generatorBase::GeneratorCustomizer &customizer
		, qReal::Id const &id
		, QObject *parent)
	: BindingGenerator(repo, customizer, id, (repo.stringProperty(id, "Filled") == "true") ? "drawing/drawFilledRect.t" : "drawing/drawRect.t"
			, {
			Binding::createConverting("@@XCoordinateRect@@", "XCoordinateRect"
					, customizer.factory()->intPropertyConverter(id, "XCoordinateRect"))
			, Binding::createConverting("@@YCoordinateRect@@", "YCoordinateRect"
					, customizer.factory()->intPropertyConverter(id, "YCoordinateRect"))
			, Binding::createConverting("@@WidthRect@@", "WidthRect"
					, customizer.factory()->intPropertyConverter(id, "WidthRect"))
			, Binding::createConverting("@@HeightRect@@", "HeightRect"
					, customizer.factory()->intPropertyConverter(id, "HeightRect"))
			}
			, parent)
{
}
