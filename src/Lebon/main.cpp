#include "main.h"

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Semantics/Analyser.h"
#include "Interpreter/Interpreter.h"

#include <windows.h>

namespace
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

                Semantics::Analyser analyser;
                if (analyser.Run(*program))
                {
                    Runtime::Interpreter interpreter;
                    interpreter.Run(*program);
                }
            }
        }

        Error::ErrorCode code = static_cast<Error::ErrorCode>(ErrorManager::Code());
        ErrorManager::Clear();

        return code;
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    // Second argument set to true also prints the tokens and the AST
    Error::ErrorCode code = RunFile("../../res/Lebon/tests/valid/program.lbn", true);

    return code == Error::ErrorCode::Ok ? 0 : 1;
}
