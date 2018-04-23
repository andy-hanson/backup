#include "effect.h"

StringSlice effect::name(Effect e) {
	switch (e) {
		case Effect::Get: return "get";
		case Effect::Set: return "set";
		case Effect::Io: return "io";
		case Effect::Own: return "own";
	}
}
