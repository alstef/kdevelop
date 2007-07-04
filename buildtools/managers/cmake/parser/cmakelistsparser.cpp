/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "cmakelistsparser.h"
#include "cmakeast.h"
// #include "cmakeprojectvisitor.h"
#include "astfactory.h"

#include <QStack>
#include <KDebug>

void CMakeFunctionDesc::addArguments( const QStringList& args )
{
    foreach( QString arg, args )
    {
        CMakeFunctionArgument cmakeArg( arg );
        arguments.append( cmakeArg );
    }
}

CMakeListsParser::CMakeListsParser(QObject *parent)
 : QObject(parent)
{
}


CMakeListsParser::~CMakeListsParser()
{
}

enum RecursivityType { No, Yes, ChangeOnly, End };

RecursivityType recursivity(const QString& functionName)
{
    if(functionName.toUpper()=="IF" || functionName.toUpper()=="WHILE" ||
           functionName.toUpper()=="FOREACH" || functionName.toUpper()=="MACRO")
        return Yes;
    else if(functionName.toUpper()=="ELSE" || functionName.toUpper()=="ELSEIF")
        return ChangeOnly;
    else if(functionName.startsWith("END"))
        return End;
    return No;
}

bool CMakeListsParser::parseCMakeFile( CMakeAst* root, const QString& fileName )
{
    if ( root == 0 )
        return false;
    cmListFileLexer* lexer = cmListFileLexer_New();
    if ( !lexer )
        return false;
    if ( !cmListFileLexer_SetFileName( lexer, qPrintable( fileName ) ) )
        return false;

    bool parseError = false;
    bool haveNewline = true;
    cmListFileLexer_Token* token;
    QStack<CMakeAst*> parent;
    parent.push(root);

    while(!parseError && (token = cmListFileLexer_Scan(lexer)))
    {
        parseError=true;
        if(token->type == cmListFileLexer_Token_Newline)
        {
            parseError=false;
            haveNewline = true;
        }
        else if(token->type == cmListFileLexer_Token_Identifier)
        {
            if(haveNewline)
            {
                CMakeAst* currentParent=parent.top();
                haveNewline = false;
                CMakeFunctionDesc function;
                function.name = token->text;
                function.filePath = fileName;
                function.line = token->line;
                
                RecursivityType s=recursivity(function.name);
                if(s==End) {
                    parent.pop();
                    break;
                } else {
                    parseError = !parseCMakeFunction( lexer, function, fileName, currentParent );
                    
                    if(!parseError && s!=No) {
                        CMakeAst* lastCreated = currentParent->children().last();
                        CMakeAst* child = new CMakeAst;
                        lastCreated->addChildAst(child);
//                         kDebug(9032) << "Lol: " << function.name << s << endl;
                        switch(s) {
                            case Yes:
                                parent.push(lastCreated);
                                break;
                            case ChangeOnly: //For else and ifelse
                                parent.pop();
                                parent.push(lastCreated);
                                break;
                            case End:
                            default:
                            case No:
                                break;
                        }
                    }
                }
            }
        }
    }

    return parseError;


}

bool CMakeListsParser::parseCMakeFunction( cmListFileLexer* lexer,
                                           CMakeFunctionDesc& func,
                                           const QString& fileName, CMakeAst *parent )
{
    // Command name has already been parsed.  Read the left paren.
    cmListFileLexer_Token* token;
    if(!(token = cmListFileLexer_Scan(lexer)))
    {
        return false;
    }
    if(token->type != cmListFileLexer_Token_ParenLeft)
    {
        return false;
    }

    // Arguments.
    unsigned long lastLine = cmListFileLexer_GetCurrentLine(lexer);
    while((token = cmListFileLexer_Scan(lexer)))
    {
        if(token->type == cmListFileLexer_Token_ParenRight)
        {
            CMakeAst* newElement = AstFactory::self()->createAst(func.name);
            bool asMacro=false;

            if(!newElement)
            {
                asMacro=true;
                newElement = new MacroCallAst;
            }
            newElement->parseFunctionInfo(func);
            parent->addChildAst(newElement);
//             kDebug(9032) << "Adding: " << func.name << newElement << " as macro :" << asMacro << endl;
            
            return true;
        }
        else if(token->type == cmListFileLexer_Token_Identifier ||
                token->type == cmListFileLexer_Token_ArgumentUnquoted)
        {
            CMakeFunctionArgument a( token->text, false, fileName, token->line );
            func.arguments << a;
        }
        else if(token->type == cmListFileLexer_Token_ArgumentQuoted)
        {
            CMakeFunctionArgument a( token->text, true, fileName, token->line );
            func.arguments << a;
        }
        else if(token->type != cmListFileLexer_Token_Newline)
        {
            return false;
        }
        lastLine = cmListFileLexer_GetCurrentLine(lexer);
    }

    return false;

}

