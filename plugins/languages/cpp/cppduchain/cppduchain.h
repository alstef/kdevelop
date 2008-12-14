/* 
   Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/**
 * This files contains common helpers for using the du-chain in the context of c++
 * */

#ifndef CPPDUCHAIN_H
#define CPPDUCHAIN_H

#include <QList>
#include <QPair>
#include "cppduchainexport.h"

namespace KTextEditor {
  class Cursor;
}

namespace KDevelop {
  class Declaration;
  class DUContext;
  class TopDUContext;
  class Identifier;
  class QualifiedIdentifier;
  class SimpleCursor;
  class IndexedString;
  class TypeIdentifier;
}

namespace Cpp {
  class EnvironmentFile;
/**
 * Searches context, and if the identifier is not found there, in its base-classes, as it should be in c++.
 * @param topContext should be the top-context from where the search starts. This is needed to resolve template-parameters.
 * Du-chain must be locked before.
 * */
KDEVCPPDUCHAIN_EXPORT QList<KDevelop::Declaration*> findLocalDeclarations( KDevelop::DUContext* context, const KDevelop::Identifier& identifier, const KDevelop::TopDUContext* topContext );

/**
 * Searches for declarations on the same level, either locally within the context, or namespace. \param identifier that will be declared in a given \param context
 * Tries to follow the C++ rules, that decide where a type may have been forward-declared.
 * */
QList<KDevelop::Declaration*> findDeclarationsSameLevel(KDevelop::DUContext* context, const KDevelop::Identifier& identifier, const KDevelop::SimpleCursor& position);

/**
 * Takes and returns a list of declarations together with inheritance-depth.
 * Since in c++ one declaration with a name in one depth overloads deeper
 * declarations, they are hidden here.
 * This also removes forward-declarations when real declarations with the same identifier are in the list.
 * */
KDEVCPPDUCHAIN_EXPORT QList< QPair<KDevelop::Declaration*, int> > hideOverloadedDeclarations( const QList< QPair<KDevelop::Declaration*, int> >& declarations );

  /**Tries determining the local class that the given code-context belongs to.
   *
   * This works within contexts of type DUContext::Other, as well as in argument contexts of type DUContext::Function(also within function definitions).
   */
KDEVCPPDUCHAIN_EXPORT KDevelop::Declaration* localClassFromCodeContext(KDevelop::DUContext* context);

/**
 * Returns whether the given declaration can be accessed from the given context. Checks for private/protected and such stuff.
 * Not implemented yet.
 * */
KDEVCPPDUCHAIN_EXPORT bool isAccessible(KDevelop::DUContext* fromContext, KDevelop::Declaration* declaration);

/**
 * Preprocesses the given string, taking the environment from the given environment-file.
 * DUChain does not need to be locked.
 * */
KDEVCPPDUCHAIN_EXPORT QString preprocess( const QString& text, EnvironmentFile* file, int line );

///Extracts a normalized signature and identifier from a specifier like "mySignal(int)"
KDEVCPPDUCHAIN_EXPORT QPair<KDevelop::Identifier, QByteArray> qtFunctionSignature(QByteArray fullFunction);

///Exchanges all occurences of @param replace in @param id with @param replaceWith
KDEVCPPDUCHAIN_EXPORT KDevelop::Identifier exchangeQualifiedIdentifier(KDevelop::Identifier id, KDevelop::QualifiedIdentifier replace, KDevelop::QualifiedIdentifier replaceWith);

///Exchanges all occurences of @param replace in @param id with @param replaceWith
KDEVCPPDUCHAIN_EXPORT KDevelop::TypeIdentifier exchangeQualifiedIdentifier(KDevelop::TypeIdentifier id, KDevelop::QualifiedIdentifier replace, KDevelop::QualifiedIdentifier replaceWith);

///Tries to un-typedef the given type using the uses directly before the given declaration.
KDEVCPPDUCHAIN_EXPORT KDevelop::TypeIdentifier unTypedefType(KDevelop::Declaration* decl, KDevelop::TypeIdentifier type);

///Returns a shortened string version of the type attached to the given declaration, using the uses to resolve typedefs and such.
///@param desiredLength the desired length. No guarantee that the resulting string will be this short.
KDEVCPPDUCHAIN_EXPORT QString shortenedTypeString(KDevelop::Declaration* decl, int desiredLength);
}

#endif