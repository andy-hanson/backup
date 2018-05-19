#pragma once

#include "../util/store/Slice.h"
#include "../util/store/StringSlice.h"
#include "./ConcreteFun.h"

struct CStructField {
	EmittableType type;
	StringSlice name;
};

struct CStruct {
	StringSlice name;
	Slice<CStructField> fields;
};

class CExpression;

struct CPropertyAccess {
	Ref<const CExpression> expression;
	bool expression_is_pointer; // True for '->', false for '.'
	Ref<const StructField> field;
};

struct CCall {
	Ref<const ConcreteFun> fun;
	Slice<CExpression> arguments;
};

class CVariableName {
public:
	enum class Kind { Temporary, Identifier };
private:
	union Data {
		uint temp_id;
		Identifier identifier;
		Data() {}
	};
	Kind _kind;
	Data _data;

public:
	inline explicit CVariableName(uint temp_id) : _kind{Kind::Temporary} { _data.temp_id = temp_id; }
	inline explicit CVariableName(Identifier identifier) : _kind{Kind::Identifier} { _data.identifier = identifier; }

	inline Kind kind() const { return _kind; }

	inline uint temp() const { return _data.temp_id; }
	inline Identifier identifier() const { return _data.identifier; }
};

// &x
struct CAddressOfExpression {
	Ref<CExpression> referenced;
};
// *x
struct CDereferenceExpression {
	Ref<CExpression> dereferenced;
};

class CExpression {
public:
	enum class Kind { VariableName, PropertyAccess, StringLiteral, AddressOf, Dereference };
private:
	union Data {
		CVariableName variable_name;
		CPropertyAccess property_access;
		StringSlice string_literal;
		CAddressOfExpression address_of;
		CDereferenceExpression dereference;

		Data() {}
	};
	Kind _kind;
	Data _data;
public:
	inline explicit CExpression(CVariableName variable_name) : _kind{Kind::VariableName} { _data.variable_name = variable_name; }
	inline explicit CExpression(CPropertyAccess property_access) : _kind{Kind::PropertyAccess} { _data.property_access = property_access; }
	inline explicit CExpression(StringSlice string_literal) : _kind{Kind::StringLiteral} { _data.string_literal = string_literal; }
	inline explicit CExpression(CAddressOfExpression address_of) : _kind{Kind::AddressOf} { _data.address_of = address_of; }
	inline explicit CExpression(CDereferenceExpression deref) : _kind{Kind::Dereference} { _data.dereference = deref; }

	inline Kind kind() const { return _kind; }
	inline const CVariableName& variable() const {
		assert(_kind == Kind::VariableName);
		return _data.variable_name;
	}
	inline const CPropertyAccess& property_access() const {
		assert(_kind == Kind::PropertyAccess);
		return _data.property_access;
	}
	inline const StringSlice& string_literal() const {
		assert(_kind == Kind::StringLiteral);
		return _data.string_literal;
	}
	inline const CAddressOfExpression& address_of() const {
		assert(_kind == Kind::AddressOf);
		return _data.address_of;
	}
	inline const CDereferenceExpression& defererence() const {
		assert(_kind == Kind::Dereference);
		return _data.dereference;
	}
};

struct CLocalDeclaration {
	EmittableType type;
	CVariableName name;
	Option<CExpression> initializer;
};

class CAssignLhs {
public:
	enum class Kind { Name, Property, Ret };
private:
	union Data {
		CVariableName name;
		CPropertyAccess property;

		Data() {}
	};
	Kind _kind;
	Data _data;
	inline explicit CAssignLhs(Kind kind) : _kind{kind} {}
public:
	inline explicit CAssignLhs(CVariableName name) : _kind{Kind::Name} { _data.name = name; }
	inline explicit CAssignLhs(CPropertyAccess property) : _kind{Kind::Property} { _data.property = property; }
	inline static CAssignLhs ret() { return CAssignLhs { Kind::Ret }; }

	inline Kind kind() const { return _kind; }
	inline const CVariableName& name() const {
		assert(_kind == Kind::Name);
		return _data.name;
	}
	inline const CPropertyAccess& property() const {
		assert(_kind == Kind::Property);
		return _data.property;
	}

	inline CExpression to_expression() const {
		switch (_kind) {
			case Kind::Name:
				return CExpression { _data.name };
			case Kind::Property:
				return CExpression { _data.property };
			case Kind::Ret:
				todo();
		}
	}
};

struct CAssignStatement {
	CAssignLhs lhs;
	CExpression expression;
};

class CStatement;

struct CIfStatement {
	CExpression condition;
	Ref<const CStatement> then;
	Ref<const CStatement> elze;
};

struct CBlockStatement {
	Slice<CStatement> statements;
};

struct CAssert {
	CExpression asserted;
};

class CStatement {
public:
	enum class Kind { Local, Assign, If, Block, Assert, Call };
private:
	union Data {
		CLocalDeclaration local;
		CAssignStatement assign;
		CIfStatement iff;
		CBlockStatement block;
		CAssert assert;
		CCall call;

		Data() {}
		~Data() {}
	};
	Kind _kind;
	Data _data;
public:
	inline CStatement(const CStatement& other) {
		*this = other;
	}
	inline void operator=(const CStatement& other) {
		_kind = other._kind;
		const Data& od = other._data;
		switch (_kind) {
			case Kind::Local:
				_data.local = od.local;
				break;
			case Kind::Assign:
				_data.assign = od.assign;
				break;
			case Kind::If:
				_data.iff = od.iff;
				break;
			case Kind::Block:
				_data.block = od.block;
				break;
			case Kind::Assert:
				_data.assert = od.assert;
				break;
			case Kind::Call:
				_data.call = od.call;
				break;
		}
	}

	inline explicit CStatement(CLocalDeclaration local) : _kind{Kind::Local} { _data.local = local; }
	inline explicit CStatement(CAssignStatement assign) : _kind{Kind::Assign} { _data.assign = assign; }
	inline explicit CStatement(CIfStatement iff) : _kind{Kind::If} { _data.iff = iff; }
	inline explicit CStatement(CBlockStatement block) : _kind{Kind::Block} { _data.block = block; }
	inline explicit CStatement(CAssert assert) : _kind{Kind::Assert} { _data.assert = assert; }
	inline explicit CStatement(CCall call) : _kind{Kind::Call} { _data.call = call; }

	inline Kind kind() const { return _kind; }
	inline const CLocalDeclaration& local() const {
		assert(_kind == Kind::Local);
		return _data.local;
	}
	inline const CAssignStatement& assign() const {
		assert(_kind == Kind::Assign);
		return _data.assign;
	}
	inline const CIfStatement& iff() const {
		assert(_kind == Kind::If);
		return _data.iff;
	}
	inline const CBlockStatement& block() const {
		assert(_kind == Kind::Block);
		return _data.block;
	}
	inline const CAssert& assert_statement() const {
		assert(_kind == Kind::Assert);
		return _data.assert;
	}
	inline const CCall& call() const {
		assert(_kind == Kind::Call);
		return _data.call;
	}
};

class CFunctionBody {
public:
	enum class Kind { Statements, Literal };
private:
	union Data {
		Slice<CStatement> statements;
		StringSlice literal;

		Data() {}
	};
	Kind _kind;
	Data _data;

public:
	inline explicit CFunctionBody(Slice<CStatement> statements) : _kind{Kind::Statements} { _data.statements = statements; }
	inline explicit CFunctionBody(StringSlice literal) : _kind{Kind::Literal} { _data.literal = literal; }

	inline Kind kind() const { return _kind; }
	inline const Slice<CStatement>& statements() const {
		assert(_kind == Kind::Statements);
		return _data.statements;
	}
	inline const StringSlice& literal() const {
		return _data.literal;
	}
};

struct CFunctionDeclaration {
	ConcreteFun fun;
	CFunctionBody body;
};

struct CStructDeclaration {
	StringSlice name;
	Slice<CStructField> fields;
};

