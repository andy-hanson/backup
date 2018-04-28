#include "effect.h"

StringSlice effect::name(Effect e) {
	switch (e) {
		case Effect::EGet: return "get";
		case Effect::ESet: return "set";
		case Effect::EIo: return "io";
		case Effect::EOwn: return "own";
	}
}
