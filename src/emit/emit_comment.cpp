#include "./emit_comment.h"

void emit_comment(Writer& writer, const Option<ArenaString>& comment) {
	if (!comment.has()) return;

	writer << "// ";
	for (char c : comment.get().slice()) {
		if (c == '\n') {
			writer << Writer::nl;
			writer << "// ";
		} else {
			writer << c;
		}
	}
	writer << Writer::nl;
}
