#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED
#include <windows.h>
LRESULT CALLBACK WindowProcedure (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

typedef void (*process_option_cb) (stringkey, stringval);
typedef void (*process_value_cb) (stringarg);

void process_arguments(char** argc, process_option_cb process_option, process_value_cb process_value);

#endif // MAIN_H_INCLUDED
