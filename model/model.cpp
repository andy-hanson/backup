#include "model.h"

const char* effect_name(Effect e) {
	switch (e) {
		case Effect::Pure: return "pure";
		case Effect::Get: return "get";
		case Effect::Set: return "set";
		case Effect::Io: return "io";
	}
}
