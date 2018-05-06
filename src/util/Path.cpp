#include "./Path.h"

#include "./PathImpl.h"

const Option<Path>& Path::parent() const {
	return impl->parent;
}
StringSlice Path::base_name() const {
	return impl->name;
}

Writer& operator<<(Writer& out, const Path& path) {
	Path::Impl::to_string_worker(out, path.impl);
	return out;
}

void Path::write(MaxSizeStringWriter& out, const StringSlice& root, Option<const StringSlice&> extension) const {
	out << root << '/';
	Path::Impl::to_string_worker(out, impl);
	if (extension.has())
		out << '.' << extension.get();
}

hash_t Path::hash::operator()(const Path& p) const {
	return Ref<const Impl>::hash{}(p.impl);
}
