#include "SYsU_lang.h" // 确保这里的头文件名与您生成的词法分析器匹配
#include <fstream>
#include <iostream>
#include <unordered_map>

int CountLine,TrueLine;
bool HasWhitespace,IsPro,IsNewLine;
auto AdString = std::to_string(-1);

// 映射定义，将ANTLR的tokenTypeName映射到clang的格式
std::unordered_map<std::string, std::string> tokenTypeMapping = {
  { "Int", "int" },
  { "Identifier", "identifier" },
  { "LeftParen", "l_paren" },
  { "RightParen", "r_paren" },
  { "RightBrace", "r_brace" },
  { "LeftBrace", "l_brace" },
  { "LeftBracket", "l_square" },
  { "RightBracket", "r_square" },
  { "Constant", "numeric_constant" },
  { "Return", "return" },
  { "Semi", "semi" },
  { "EOF", "eof" },
  { "Equal", "equal" },
  { "Plus", "plus" },
  { "Comma", "comma" },

  // 在这里继续添加其他映射
  { "Const", "const" },
  { "Minus", "minus" },
  { "Star", "star" },
  { "Slash", "slash" },
  { "Percent", "percent" },
  { "Greater", "greater" },
  { "Less", "less" },
  { "If", "if" },
  { "Else", "else" },
  { "Equalequal", "equalequal" },
  { "Void", "void" },
  { "While", "while" },
  { "Break", "break" },
  { "Continue", "continue" },
  { "Pipepipe", "pipepipe" },
  { "Ampamp", "ampamp" },
  { "Lessequal", "lessequal" },
  { "Greaterequal", "greaterequal" },
  { "Exclaimequal", "exclaimequal" },
  { "Exclaim", "exclaim" },
};

void
print_token(const antlr4::Token* token,
            const antlr4::CommonTokenStream& tokens,
            std::ofstream& outFile,
            const antlr4::Lexer& lexer)
{
  auto& vocabulary = lexer.getVocabulary();

  auto tokenTypeName =
    std::string(vocabulary.getSymbolicName(token->getType()));

  if (tokenTypeName.empty())
    tokenTypeName = "<UNKNOWN>"; // 处理可能的空字符串情况

  if (tokenTypeMapping.find(tokenTypeName) != tokenTypeMapping.end()) {
    tokenTypeName = tokenTypeMapping[tokenTypeName];
  }

  auto LocPos =
    std::to_string(token->getCharPositionInLine()+1);
  
  if(tokenTypeName == "LineAfterPreprocessing")
  {
    IsPro = true;
    auto AdressString = token->getText();
    TrueLine = 0;
    for(int i=0;i<AdressString.length();++i)
    {
      if(AdressString[i]>='0'&&AdressString[i]<='9')
      {
        while(AdressString[i]>='0'&&AdressString[i]<='9')
        {
          TrueLine = TrueLine * 10 + AdressString[i] - '0';
          i++;
        }
        break;
      }
    }

    int ll=0,rr;
    for(int i=0;i<AdressString.length();++i)
    {
      if(AdressString[i]=='"')
      {
        if(!ll)
          ll=i+1;
        else
          rr=i-1;
      }
    }
    AdString = AdressString.substr(ll,rr-ll+1);

    //outFile << AdressString << "**" <<AdString << "**" << TrueLine << std::endl;    
    return ;
  }
  else if(IsPro == true && LocPos == "1")
  {
    IsPro = false;
    //IsNotPro = true;
    CountLine = TrueLine;
    IsNewLine = true;
  }

  if(tokenTypeName == "Whitespace" && IsPro == false)
  {
    HasWhitespace=true;
    return ;
  }

  if(tokenTypeName == "Newline")
  {
    CountLine++;
    IsNewLine = true;
    return ;
  }

  bool startOfLine = false;
  bool leadingSpace = false;

  if(IsNewLine)
  {
    startOfLine = true;
    IsNewLine = false;
  }
  if(HasWhitespace)
  {
    leadingSpace = true;
    HasWhitespace = false;
  }

  std::string locInfo = " Loc=<" + AdString + ":" + std::to_string(CountLine) + ":" + LocPos + ">";

  if (token->getText() != "<EOF>")
    outFile << tokenTypeName << " '" << token->getText() << "'";
  else
    outFile << tokenTypeName << " '"
            << "'";
  if (startOfLine)
    outFile << "\t [StartOfLine]";
  if (leadingSpace)
    outFile << " [LeadingSpace]";
  outFile << locInfo << std::endl;
}

int
main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <input> <output>\n";
    return -1;
  }

  std::ifstream inFile(argv[1]);
  if (!inFile) {
    std::cout << "Error: unable to open input file: " << argv[1] << '\n';
    return -2;
  }

  std::ofstream outFile(argv[2]);
  if (!outFile) {
    std::cout << "Error: unable to open output file: " << argv[2] << '\n';
    return -3;
  }

  std::cout << "程序 '" << argv[0] << std::endl;
  std::cout << "输入 '" << argv[1] << std::endl;
  std::cout << "输出 '" << argv[2] << std::endl;

  antlr4::ANTLRInputStream input(inFile);
  SYsU_lang lexer(&input);

  antlr4::CommonTokenStream tokens(&lexer);
  tokens.fill();

  CountLine = 1;
  HasWhitespace = false;
  IsPro = true;
  //IsNotPro = false;
  IsNewLine = true;

  for (auto&& token : tokens.getTokens()) {
    print_token(token, tokens, outFile, lexer);
  }
}
