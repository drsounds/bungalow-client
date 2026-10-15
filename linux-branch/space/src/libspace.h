#pragma once
#include <string>

#include "SPColor.h"

#include <map>
#define DEBUG
#define CONTROL_INPUT 1
#define SP_EVENT_MOUSE_DOWN 1
#define SP_EVENT_MOUSE_UP 2
#define SP_EVENT_MOUSE_MOVE 3
#define SP_EVENT_NAVIGATE 4

using namespace std;

namespace space {
	class SPElement;
	class SPObject;
	class SPEventArgs;
	class SPEvent
	{
	public:
		string type;
		SPEventArgs *args;
		SPObject *sender;
		SPEvent(const string& type, SPEventArgs *args, SPObject *sender) {
			this->type = type;
			this->args = args;
			this->sender = sender;
		}
	};
	class SPMessage
	{
	public:
		string type;
		string subject_uri;
		string predicate;
		string object_uri;
		string attribute;
		string value;
	};
	struct sp_margin {
		int left;
		int top;
		int bottom;
		int right;
	};
	int htmlToColor(string str);
	struct sp_position {
		int x;
		int y;
	};
	struct sp_rectangle {
		int x;
		int y;
		int width;
		int height;
		int z;
	};

	typedef int(SPElement::*SPCallback)(SPObject *, SPEventArgs *);
}
