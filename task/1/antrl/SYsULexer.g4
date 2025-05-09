lexer grammar SYsU_lang;

Int : 'int';
Return : 'return';
If : 'if';
Else : 'else';
Void : 'void';
While : 'while';
Break : 'break';
Continue : 'continue';
Pipepipe : '||';
Ampamp : '&&';


LeftParen : '(';
RightParen : ')';
LeftBracket : '[';
RightBracket : ']';
LeftBrace : '{';
RightBrace : '}';
Const : 'const';

Plus : '+';
Minus : '-';
Star : '*';
Slash : '/';
Percent : '%';
Greater : '>';
Less : '<';

Semi : ';';
Comma : ',';

Equalequal : '==';
Equal : '=';
Lessequal : '<=';
Greaterequal : '>=';
Exclaimequal : '!=';
Exclaim : '!';


Identifier
    :   IdentifierNondigit
        (   IdentifierNondigit
        |   Digit
        )*
    ;

fragment
IdentifierNondigit
    :   Nondigit
    ;

fragment
Nondigit
    :   [a-zA-Z_]
    ;

fragment
Digit
    :   [0-9]
    ;

Constant
    :   IntegerConstant
    ;

fragment
IntegerConstant
    :   DecimalConstant
    |   OctalConstant
    |   HexadecimalConstant
    ;

fragment
DecimalConstant
    :   NonzeroDigit Digit*
    ;

fragment
OctalConstant
    :   '0' OctalDigit*
    ;
HexadecimalConstant
    :   '0x' HexadecimalDigit*
    ;

fragment
NonzeroDigit
    :   [1-9]
    ;

fragment
OctalDigit
    :   [0-7]
    ;
HexadecimalDigit
    :   [0-9]
    |   [a-f]
    ;


// 预处理信息处理，可以从预处理信息中获得文件名以及行号
// 预处理信息前面的数组即行号
LineAfterPreprocessing
    :   '#' Whitespace* ~[\r\n]*
        
    ;

Whitespace
    :   [ \t]+
        
    ;

// 换行符号，可以利用这个信息来更新行号
Newline
    :   (   '\r' '\n'?
        |   '\n'
        )
    ;

