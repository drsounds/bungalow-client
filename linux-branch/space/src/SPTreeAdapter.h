#pragma once
#include "libspace.h"
namespace space {
    template <class T>
	class SPTreeAdapter {
	public:
		virtual T getItemAt(int pos);
		virtual int count();
	};
}

