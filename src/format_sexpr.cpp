/*
** Author(s):
**  - Cedric GESTES  <gestes@aldebaran-robotics.com>
**
** Copyright (C) 2014 Aldebaran Robotics
*/

#include <sstream>
#include <stdexcept>
#include <qilang/node.hpp>
#include <qilang/formatter.hpp>
#include "formatter_p.hpp"


namespace qilang {

  // #############
  // CONST DATA
  // #############
  class ASTConstDataFormatter : public ConstDataNodeFormatter {
  public:
    virtual void acceptData(const ConstDataNodePtr& node) { node->accept(this); }

    const std::string &list(ConstDataNodePtrVector pv) {
      static const std::string ret;
      for (int i = 0; i < pv.size(); ++i) {
        cdata(pv.at(i));
        if (i + 1 < pv.size())
          out() << " ";
      }
      return ret;
    }

    const std::string &dict(ConstDataNodePtrPairVector pv) {
      static const std::string ret;
      for (int i = 0; i < pv.size(); ++i) {
        out() << "(" << cdata(pv.at(i).first) << " " << cdata(pv.at(i).second) << ")";
        if (i + 1 < pv.size())
          out() << " ";
      }
      return ret;
    }
    void visitData(BoolConstDataNode *node) {
      out() << "(bool " << node->value << ")";
    }
    void visitData(IntConstDataNode *node) {
      out() << "(int " << node->value << ")";
    }
    void visitData(FloatConstDataNode *node) {
      out() << "(float " << node->value << ")";
    }
    void visitData(StringConstDataNode *node) {
      out() << "(string " << node->value << ")";
    }
    void visitData(ListConstDataNode* node) {
      out() << "(list " << list(node->values) << ")";
    }
    void visitData(TupleConstDataNode* node) {
      out() << "(tuple " << list(node->values) << ")";
    }
    void visitData(DictConstDataNode* node) {
      out() << "(dict " << dict(node->values) << ")";
    }
  };

  // #############
  // TYPE EXPR
  // #############
  class ASTTypeExprFormatter : public TypeExprNodeFormatter {
  public:
    virtual void acceptTypeExpr(const TypeExprNodePtr& node)  { node->accept((TypeExprNodeVisitor*)this); }
    void visitTypeExpr(SimpleTypeExprNode *node) {
      out() << "(type " << node->value << ")";
    }
    void visitTypeExpr(ListTypeExprNode *node) {
      out() << "(listtype " << type(node->element) << ")";
    }
    void visitTypeExpr(MapTypeExprNode *node) {
      out() << "(maptype " << type(node->key) << " " << type(node->value) << ")";
    }
    void visitTypeExpr(TupleTypeExprNode *node) {
      out() << "(tupletype ";
      for (int i = 0; i < node->elements.size(); ++i) {
        out() << type(node->elements.at(i));
        if (i + 1 == node->elements.size())
          out() << " ";
      }
      out() << ")";
    }
  };

  // #############
  // EXPR
  // #############
  class ASTExprFormatter : virtual public ASTConstDataFormatter, public ExprNodeFormatter {
  public:
    virtual void acceptExpr(const ExprNodePtr& node) { node->accept((ExprNodeVisitor*)this); }

    void visitExpr(BinaryOpExprNode *node) {
      out() << "(" << BinaryOpCodeToString(node->op) << " " << expr(node->n1) << " " << expr(node->n2) << ")";
    }
    void visitExpr(UnaryOpExprNode *node) {
      out() << "(" << UnaryOpCodeToString(node->op) << " " << expr(node->n1) << ")";
    }
    void visitExpr(VarExprNode *node) {
       out() << "(var " << node->value << ")";
    }
    void visitExpr(ConstDataExprNode* node) {
      out() << cdata(node->data);
    }
  };

  // #############
  // DECL
  // #############
  class ASTDeclFormatter: virtual public ASTTypeExprFormatter, virtual public ASTConstDataFormatter, public DeclNodeFormatter {
  public:
    virtual void acceptDecl(const DeclNodePtr& node) { node->accept((DeclNodeVisitor*)this); }

    void visitDecl(InterfaceDeclNode* node) {
      indent() << "(interface " << node->name;
      if (node->inherits.size() > 0) {
        out() << "(inherit ";
        for (int i = 0; i < node->inherits.size(); ++i) {
          out() << node->inherits.at(i);
          if (i + 1 != node->inherits.size())
            out() << " ";
        }
        out() << ")";
      }
      out() << std::endl;
      scopedDecl(node->values);
      indent() << ")" << std::endl;
    }

    void declParamList(const std::string &declname, const std::string& name, const TypeExprNodePtrVector& vec, const TypeExprNodePtr &ret = TypeExprNodePtr()) {
      out() << "(" << declname << " " << name << "(";
      for (unsigned int i = 0; i < vec.size(); ++i) {
        out() << type(vec[i]);
        if (i+1 < vec.size()) {
          out() << " ";
        }
      }
      out() << ")";
      if (ret)
        out() << " " << type(ret);
      out() << ")" << std::endl;
    }

    void visitDecl(FnDeclNode* node) {
      declParamList("fn", node->name, node->args, node->ret);
    }
    void visitDecl(EmitDeclNode* node) {
      declParamList("out", node->name, node->args);
    }
    void visitDecl(PropDeclNode* node) {
      declParamList("prop", node->name, node->args);
    }

    void visitDecl(StructDeclNode* node) {
      indent() << "(struct " << node->name << std::endl;
      scopedField(node->fields);
      indent() << ")" << std::endl;
    }

    void visitDecl(FieldDeclNode* node) {
      indent() << "(field " << node->name;
      if (node->type)
        out() << " " << type(node->type);
      out() << ")" << std::endl;
    }

    void visitDecl(ConstDeclNode* node) {
      indent() << "(defconst " << node->name;
      if (node->type)
        out() << " " << type(node->type);
      if (node->data)
        out() << " " << cdata(node->data);
      out() << ")" << std::endl;
    }

  };

  class ASTStmtFormatter: virtual public ASTTypeExprFormatter, virtual public ASTConstDataFormatter, public StmtNodeFormatter {
  public:
    virtual void acceptStmt(const StmtNodePtr& node) { node->accept((StmtNodeVisitor*)this); }

    void visitStmt(PackageNode* node) {
      indent() << "(package " << node->name << ")" << std::endl;
    }

    void visitStmt(ImportNode* node) {
      if (node->imported.size() == 0) {
        indent() << "(import " << node->name << ")" << std::endl;
      } else {
        indent() << "(from " << node->name << " (import ";
        for (int i = 0; i < node->imported.size(); ++i) {
          out() << node->imported.at(i);
          if (i+1 < node->imported.size()) {
            out() << " ";
          }
        }
        out() << "))" << std::endl;
      }
    }

    void visitStmt(ObjectDefNode *node) {
      indent() << "(object " << type(node->type) << " " << node->name << std::endl;
      scopedStmt(node->values);
      indent() << ")" << std::endl;
    }
    void visitStmt(PropertyDefNode *node) {
      indent() << "(prop " << node->name << " " << cdata(node->data) << ")" << std::endl;
    }
    void visitStmt(AtNode* node) {
      indent() << "(at " << node->sender << " " << node->receiver << ")" << std::endl;
    }
    void visitStmt(VarDefNode* node) {
      indent() << "(defvar " << node->name;
      if (node->type)
        out() << " " << type(node->type);
      if (node->data)
        out() << " " << cdata(node->data);
      out() << ")" << std::endl;
    }

  };

  class QiLangASTFormatter : public FileFormatter
                           , public ASTStmtFormatter
                           , public ASTDeclFormatter
                           , public ASTExprFormatter
  {
  protected:
    virtual void accept(const NodePtr& node) {
      switch (node->kind()) {
      case NodeKind_ConstData:
        acceptData(boost::dynamic_pointer_cast<ConstDataNode>(node));
        break;
      case NodeKind_Decl:
        acceptDecl(boost::dynamic_pointer_cast<DeclNode>(node));
        break;
      case NodeKind_Expr:
        acceptExpr(boost::dynamic_pointer_cast<ExprNode>(node));
        break;
      case NodeKind_Stmt:
        acceptStmt(boost::dynamic_pointer_cast<StmtNode>(node));
        break;
      case NodeKind_TypeExpr:
        acceptTypeExpr(boost::dynamic_pointer_cast<TypeExprNode>(node));
        break;
      }
    }
  };

  std::string formatAST(const NodePtr& node) {
    return QiLangASTFormatter().format(node);
  }

  std::string formatAST(const NodePtrVector& node) {
    return QiLangASTFormatter().format(node);
  }

}
