#include "./CAst_emit.h"

namespace {
	void write_type(Writer& out, const EmittableType& e, const Names& names) {
		out << names.name(e.inst_struct);
		if (e.is_pointer) out << '*';
	}

	void write(Writer& out, const CVariableName& name) {
		switch (name.kind()) {
			case CVariableName::Kind::Identifier:
				out << name.identifier();
				break;
			case CVariableName::Kind::Temporary:
				out << "_tmp_" << name.temp();
				break;
		}
	}

	void write(Writer& out, const CExpression& e, const Names& names);

	void write(Writer& out, const CPropertyAccess& p, const Names& names) {
		write(out, p.expression, names);
		out << (p.expression_is_pointer ? "->" : ".");
		out << names.name(p.field);
	}

	void write_string_literal(Writer& out, const StringSlice& slice) {
		out << '"';
		for (char c : slice) {
			switch (c) {
				case '\n':
					out << "\\n";
					break;
				case '"':
					out << "\\\"";
					break;
				default:
					out << c;
					break;
			}
		}
		out << '"';
	}

	void write(Writer& out, const CExpression& e, const Names& names) {
		switch (e.kind()) {
			case CExpression::Kind::AddressOf:
				out << '&';
				write(out, e.address_of().referenced, names);
				break;
			case CExpression::Kind::Dereference:
				out << '*';
				write(out, e.defererence().dereferenced, names);
				break;
			case CExpression::Kind::PropertyAccess:
				write(out, e.property_access(), names);
				break;
			case CExpression::Kind::StringLiteral:
				write_string_literal(out, e.string_literal());
				break;
			case CExpression::Kind::VariableName:
				write(out, e.variable());
				break;
		}
	}

	void write(Writer& out, const CAssignLhs& a, const Names& names) {
		switch (a.kind()) {
			case CAssignLhs::Kind::Name:
				write(out, a.name());
				break;
			case CAssignLhs::Kind::Property:
				write(out, a.property(), names);
				break;
			case CAssignLhs::Kind::Ret:
				out << "_ret";
				break;
		}
	}

	void write(Writer& out, const CStatement& s, const Names& names) {
		switch (s.kind()) {
			case CStatement::Kind::Assign: {
				const CAssignStatement& as = s.assign();
				write(out, as.lhs, names);
				out << " = ";
				write(out, as.expression, names);
				out << ';';
				break;
			}
			case CStatement::Kind::Assert: {
				out << "assert(";
				write(out, s.assert_statement().asserted, names);
				out << ");";
				break;
			}
			case CStatement::Kind::Block: {
				out << "{" << Writer::indent;
				for (const CStatement& sub_statement : s.block().statements) {
					out << Writer::nl;
					write(out, sub_statement, names);
				}
				out << Writer::dedent << Writer::nl;
				break;
			}
			case CStatement::Kind::Call: {
				const CCall& call = s.call();
				out << names.name(call.fun) << '(';
				bool first_arg = true;
				for (const CExpression& e : call.arguments) {
					if (first_arg) first_arg = false; else out << ", ";
					write(out, e, names);
				}
				out << ");";
				break;
			}
			case CStatement::Kind::If: {
				const CIfStatement& iff = s.iff();
				out << "if (";
				write(out, iff.condition, names);
				out << ") " << Writer::indent;
				write(out, iff.then, names);
				out << " else ";
				write(out, iff.elze, names);
				out << Writer::dedent << Writer::nl;
				break;
			}
			case CStatement::Kind::Local: {
				const CLocalDeclaration& local = s.local();
				write_type(out, local.type, names);
				out << ' ';
				write(out, local.name);
				if (local.initializer.has()) {
					out << " = ";
					write(out, local.initializer.get(), names);
				}
				out << ';';
				break;
			}
		}
	}

	void write_indented(Writer& out, const StringSlice& s) {
		for (char c : s) {
			if (c == '\n')
				out << Writer::nl;
			else
				out << c;
		}
	}
}

void write_emittable_struct(Writer& out, const EmittableStruct& e, const Names& names) {
	const StructBody& body = e.strukt->body;
	switch (body.kind()) {
		case StructBody::Kind::Nil: unreachable();
		case StructBody::Kind::CppName:
			out << "typedef " << body.cpp_name() << ' ' << names.name(e) << ';';
			break;
		case StructBody::Kind::Fields:
			out << "struct " << names.name(e) << " {" << Writer::indent << Writer::nl;
			zip(body.fields(), e.field_types, [&](const StructField& field, const EmittableType& field_type) {
				write_type(out, field_type, names);
				out << ' ';
				out << names.name(field);
			});
			out << Writer::dedent << Writer::nl << "};";
	}
}

void write_fun_header(Writer& out, const ConcreteFun& cf, const Names& names) {
	out << "void " << names.name(cf) << '(';
	write_type(out, cf.return_type, names);
	out << "* _ret";
	zip(cf.parameter_types, cf.fun_declaration->signature.parameters, [&](const EmittableType& parameter_type, const Parameter& parameter) {
		out << ", ";
		write_type(out, parameter_type, names);
		out << ' ' << names.name(parameter);
	});
	out << ')';
}

void write_fun_implementation(Writer& out, const ConcreteFun& cf, const CFunctionBody& body, const Names& names) {
	write_fun_header(out, cf, names);
	out << " {" << Writer::indent;
	switch (body.kind()) {
		case CFunctionBody::Kind::Statements:
			for (const CStatement& s : body.statements()) {
				out << Writer::nl;
				write(out, s, names);
			}
			break;
		case CFunctionBody::Kind::Literal:
			write_indented(out, body.literal());
	}
	out << Writer::dedent << Writer::nl;
	out << "}";
}
