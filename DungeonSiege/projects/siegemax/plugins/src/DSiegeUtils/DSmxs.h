#pragma once
#ifndef DSMXS_H
#define DSMXS_H

#define	is_undefined(v) ((v)->tag == class_tag(Undefined))

//class Value;
Value* RunMaxScriptCommand(TCHAR* command);

#endif