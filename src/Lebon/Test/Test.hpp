#ifndef TEST_HPP_DEFINED
#define TEST_HPP_DEFINED

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Semantics/Analyser.h"

namespace Test
{
    // Runs the lexer, the parser and the analyser on one file.
    // A stage only runs if the previous ones logged no error.
    // Returns the code of the first error, Ok if the file went through.
    Error::ErrorCode RunFile(fs::path const& _path, bool _verbose)
    {
        Lexer lexer(_path);
        lexer.Scan();
        if (_verbose)
            lexer.DisplayTokens();

        if (ErrorManager::HasErrors() == false)
        {
            Parser parser(lexer.GetTokens());
            std::unique_ptr<Program> program = parser.Parse();

            if (program && ErrorManager::HasErrors() == false)
            {
                Semantics::Analyser analyser;
                analyser.Run(*program);

                // Printed after the analysis so the types and symbol ids are filled
                if (_verbose)
                {
                    AstPrinter printer;
                    printer.Print(*program);
                }
            }
        }

        Error::ErrorCode code = static_cast<Error::ErrorCode>(ErrorManager::Code());
        ErrorManager::Clear();

        return code;
    }
}

#endif