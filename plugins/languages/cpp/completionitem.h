/*
 * KDevelop C++ Code Completion Support
 *
 * Copyright 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef COMPLETIONITEM_H
#define COMPLETIONITEM_H

#include <ksharedptr.h>
#include <ktexteditor/codecompletionmodel.h>

#include <language/duchain/duchainpointer.h>
#include <language/codecompletion/codecompletionitem.h>
#include <codecompletioncontext.h>
#include "includeitem.h"

namespace Cpp {
  class CodeCompletionContext;
}

namespace KTextEditor {
  class CodeCompletionModel;
  class Document;
  class Range;
  class Cursor;
}

class QModelIndex;
class CppCodeCompletionModel;

//A completion item used for completion of normal declarations while normal code-completion
class NormalDeclarationCompletionItem : public KDevelop::CompletionTreeItem {
public:
  NormalDeclarationCompletionItem(KDevelop::DeclarationPointer decl = KDevelop::DeclarationPointer(), KSharedPtr<Cpp::CodeCompletionContext> context=KSharedPtr<Cpp::CodeCompletionContext>(), int _inheritanceDepth = 0, int _listOffset=0) : m_declaration(decl), completionContext(context), m_inheritanceDepth(_inheritanceDepth), listOffset(_listOffset), useAlternativeText(false), m_isQtSignalSlotCompletion(false), m_fixedMatchQuality(-1) {
  }
  
  virtual void execute(KTextEditor::Document* document, const KTextEditor::Range& word);

  virtual QVariant data(const QModelIndex& index, int role, const KDevelop::CodeCompletionModel* model) const;

  virtual KDevelop::IndexedType typeForArgumentMatching() const;
  
  KDevelop::DeclarationPointer m_declaration;
  KSharedPtr<Cpp::CodeCompletionContext> completionContext;
  int m_inheritanceDepth; //Inheritance-depth: 0 for local functions(within no class), 1 for within local class, 1000+ for global items.
  int listOffset; //If it is an argument-hint, this contains the offset within the completion-context's function-list
  QString alternativeText; //Text shown when declaration is zero
  
  //If this is true, alternativeText will be shown in the list, and will be inserted on execution.
  //Also the scope will be set to LocalScope when this attribute is true.
  bool useAlternativeText;

  //If this is true, the execution of the item should trigger the insertion of a complete SIGNAL/SLOT use
  bool m_isQtSignalSlotCompletion;

  //If this is not -1, it can be a fixed match-quality from 0 to 10, that will be used non-dynamically.
  int m_fixedMatchQuality;
  
  KTextEditor::CodeCompletionModel::CompletionProperties completionProperties() const;
  
  virtual KDevelop::DeclarationPointer declaration() const {
    return m_declaration;
  }
  
  virtual int inheritanceDepth() const;
  virtual int argumentHintDepth() const;
};

//A completion item used for completing include-files
class IncludeFileCompletionItem : public KDevelop::CompletionTreeItem {
public:
  IncludeFileCompletionItem(const Cpp::IncludeItem& include) : includeItem(include) {
  }

  virtual QVariant data(const QModelIndex& index, int role, const KDevelop::CodeCompletionModel* model) const;

  virtual void execute(KTextEditor::Document* document, const KTextEditor::Range& word);

  virtual int inheritanceDepth() const;
  virtual int argumentHintDepth() const;

  Cpp::IncludeItem includeItem;
};

class TypeConversionCompletionItem : public KDevelop::CompletionTreeItem {
  public:
    TypeConversionCompletionItem(QString text, KDevelop::IndexedType type, int argumentHintDepth);
    virtual int argumentHintDepth() const;
    virtual QVariant data(const QModelIndex& index, int role, const KDevelop::CodeCompletionModel* model) const;
    KDevelop::IndexedType type() const;
    virtual KDevelop::IndexedType typeForArgumentMatching() const;
  private:
    QString m_text;
    KDevelop::IndexedType m_type;
    int m_argumentHintDepth;
};

//A completion item used for completing include-files
/*class ArgumentListCompletionItem : public KDevelop::CompletionTreeItem {
public:
  IncludeFileCompletionItem(const Cpp::IncludeItem& include) : includeItem(include) {
  }

  virtual QVariant data(const QModelIndex& index, int role, const KDevelop::CodeCompletionModel* model) const;

  virtual void execute(KTextEditor::Document* document, const KTextEditor::Range& word);

  virtual int inheritanceDepth() const;
  virtual int argumentHintDepth() const;

  Cpp::IncludeItem includeItem;
};*/
#endif