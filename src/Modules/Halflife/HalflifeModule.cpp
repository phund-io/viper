#include "BoostPythonSM.h"
#include "HalflifeModule.h"
#include "sdk/smsdk_ext.h"
#include "Macros.h"

namespace py = boost::python;

namespace Viper {
	namespace Modules {
		namespace Halflife {
			py::object halflife_get_game_description(py::tuple argumentsTuple,
				py::dict keywordsDict) {
				bool original = false;
					
				if(keywordsDict.contains("original")) {
					original = py::extract<bool>(keywordsDict["original"]);
				}

				if(original) {
					return py::str(gamedll->GetGameDescription());
				}

				return py::str(SH_CALL(gamedllPatch, &IServerGameDLL::GetGameDescription)());
			}

			BOOST_PYTHON_MODULE(halflife) {
				py::def("get_game_description", py::raw_function(halflife_get_game_description,
					0));
			}
		}
	}
}