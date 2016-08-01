#include "stdafx.h"
#include "codegen.h"
#include "svm_codes.h"

namespace halang
{

	CodeGen::CodeGen(Parser& _p) : _parser(_p)
	{ 
		top_cp = global_cp = new CodePack();
	}

	void CodeGen::generate()
	{
		generate(global_cp);

		global_cp->instructions.push_back(Instruction(VM_CODE::STOP, 0));
	}

	void CodeGen::generate(CodePack* cp)
	{
		visit(cp, _parser.getRoot());
	}

	void CodeGen::visit(CodePack* cp, Node* _node)
	{
		_node->visit(this, cp);
		/*
		if (_node == nullptr)
			return;
		else if (_node->asIdentifier())
			visit(cp, _node->asIdentifier());
		else if (_node->asNumber())
			visit(cp, _node->asNumber());
		else if (_node->asAssignment())
			visit(cp, _node->asAssignment());
		else if (cp, _node->asBinaryExpression())
			visit(cp, _node->asBinaryExpression());
		else if (_node->asUnaryExpression())
			visit(cp, _node->asUnaryExpression());
		else if (cp, _node->asBlockExpression())
			visit(cp, _node->asBlockExpression());
		else if (cp, _node->asIfStmt())
			visit(cp, _node->asIfStmt());
		else if (_node->asWhileStmt())
			visit(cp, _node->asWhileStmt());
		else if (cp, _node->asBreakStmt())
			visit(cp, _node->asBreakStmt());
		else if (cp, _node->asReturnStmt())
			visit(cp, _node->asReturnStmt());
		else if (_node->asFuncDef())
			visit(cp, _node->asFuncDef());
			*/
	}

	void CodeGen::visit(CodePack* cp, BlockExprNode* _node)
	{
		for (auto i = _node->children.begin(); i != _node->children.end(); ++i)
		{
			(*i)->visit(this, cp);

			// TODO: debug
			if ((*i)->asUnaryExpression() || 
				(*i)->asBinaryExpression() || 
				(*i)->asIdentifier() || 
				(*i)->asNumber())
				cp->instructions.push_back(Instruction(VM_CODE::OUT, 0));
		}
	}

	void CodeGen::visit(CodePack* cp, UnaryExprNode* _node)
	{
		visit(cp, _node->child);
		switch (_node->op)
		{
		case OperatorType::SUB:
			cp->instructions.push_back(Instruction(VM_CODE::PUSH_INT, -1));
			cp->instructions.push_back(Instruction(VM_CODE::MUL, 0));
			break;
		case OperatorType::NOT:
			cp->instructions.push_back(Instruction(VM_CODE::NOT, 0));
			break;
		}
	}

	void CodeGen::visit(CodePack* cp, BinaryExprNode* _node)
	{
		visit(cp, _node->left);
		visit(cp, _node->right);
		switch (_node->op)
		{
		case OperatorType::ADD:
			cp->instructions.push_back(Instruction(VM_CODE::ADD, 0));
			break;
		case OperatorType::SUB:
			cp->instructions.push_back(Instruction(VM_CODE::SUB, 0));
			break;
		case OperatorType::MUL:
			cp->instructions.push_back(Instruction(VM_CODE::MUL, 0));
			break;
		case OperatorType::DIV:
			cp->instructions.push_back(Instruction(VM_CODE::DIV, 0));
			break;
		case OperatorType::MOD:
			cp->instructions.push_back(Instruction(VM_CODE::MOD, 0));
			break;
		case OperatorType::POW:
			cp->instructions.push_back(Instruction(VM_CODE::POW, 0));
			break;
		case OperatorType::GT:
			cp->instructions.push_back(Instruction(VM_CODE::GT, 0));
			break;
		case OperatorType::LT:
			cp->instructions.push_back(Instruction(VM_CODE::LT, 0));
			break;
		case OperatorType::GTEQ:
			cp->instructions.push_back(Instruction(VM_CODE::GTEQ, 0));
			break;
		case OperatorType::LTEQ:
			cp->instructions.push_back(Instruction(VM_CODE::LTEQ, 0));
			break;
		case OperatorType::EQ:
			cp->instructions.push_back(Instruction(VM_CODE::EQ, 0));
			break;
		default:
			// runtime error
			cp->instructions.push_back(Instruction(VM_CODE::POP, 0));
			// pack.instructions.push_back(Instruction(VM_CODE::POP, 0));
		}
	}

	void CodeGen::visit(CodePack* cp, NumberNode* _node)
	{
		auto index = cp->constant.size();

		if (_node->maybeInt)
			cp->constant.push_back(Object(static_cast<int>(_node->number)));
		cp->constant.push_back(Object(_node->number));

		cp->instructions.push_back(Instruction(VM_CODE::LOAD_C, index));
	}

	void CodeGen::visit(CodePack* cp, IdentifierNode* _node)
	{
		// read the value of the memory
		// and push to the top of the stack

		for (auto i = cp->var_names.begin(); i != cp->var_names.end(); ++i)
			if (i->second == _node->name)
			{
				cp->instructions.push_back(Instruction(VM_CODE::LOAD_V, i->first));
				return;
			}
		// codegen error;
	}

	void CodeGen::visit(CodePack* cp, AssignmentNode* _node)
	{
		// find if the var is exisits
		// if not exisits add a possition for it
		auto _id = dynamic_cast<IdentifierNode*>(_node->identifier);

		bool found = false;
		for (auto i = cp->var_names.begin(); i != cp->var_names.end(); ++i)
		{
			if (i->second == _id->name)
			{
				found = true;
				visit(cp, _node->expression);
				cp->instructions.push_back(Instruction(VM_CODE::STORE_V, i->first));
				return;
			}
		}

		// TODO: codegen error
	}

	void CodeGen::visit(CodePack* cp, IfStmtNode* _node)
	{
		int jmp_val;
		visit(cp, _node->condition);
		auto jmp_loc = cp->instructions.size();
		cp->instructions.push_back(Instruction(VM_CODE::IFNO, 1));
		visit(cp, _node->true_branch);
		auto true_finish_loc = cp->instructions.size();
		cp->instructions.push_back(Instruction(VM_CODE::JMP, 1));
		// if condition not ture, jmp to the right location
		jmp_val = cp->instructions.size() - jmp_loc;
		cp->instructions[jmp_loc] = Instruction(VM_CODE::IFNO, jmp_val); 
		if (_node->false_branch)
		{
			visit(cp, _node->false_branch);
			jmp_val = cp->instructions.size() - true_finish_loc;
			cp->instructions[true_finish_loc] = Instruction(VM_CODE::JMP, jmp_val);
		}
	}

	void CodeGen::visit(CodePack* cp, WhileStmtNode* _node)
	{
		auto _begin_loc = cp->instructions.size();
		visit(cp, _node->condition);
		auto _condition_loc = cp->instructions.size();
		cp->instructions.push_back(Instruction(VM_CODE::IFNO, 0));
		visit(cp, _node->child);
		cp->instructions.push_back(Instruction(VM_CODE::JMP, -1 * (cp->instructions.size() - _begin_loc)));
		cp->instructions[_condition_loc] = Instruction(VM_CODE::IFNO, cp->instructions.size() - _condition_loc);
	}

	void CodeGen::visit(CodePack* cp, BreakStmtNode* _node)
	{

	}

	void CodeGen::visit(CodePack* cp, ReturnStmtNode* _node)
	{

	}

	void CodeGen::visit(CodePack* cp, FuncDefNode* _node)
	{
		auto new_pack = new CodePack();
		new_pack->prev = top_cp;
		visit(new_pack, _node->block);
	}

}
