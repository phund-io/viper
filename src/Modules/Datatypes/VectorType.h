#ifndef __INCLUDE_VECTORTYPE_H__
#define __INCLUDE_VECTORTYPE_H__

#include "Macros.h"

namespace Viper {
	namespace Modules {
		namespace Datatypes {
			class VectorType {
			public:
				int Compare(VectorType v);
				bool CompareEqual(VectorType v);
				bool CompareNotEqual(VectorType v);
				bool CompareGreaterThan(VectorType v);
				bool CompareLessThan(VectorType v);
				bool CompareGreaterThanOrEqual(VectorType v);
				bool CompareLessThanOrEqual(VectorType v);
				std::string ReprMagic();

				float x;
				float y;
				float z;
			};
		}
	}
}

#endif