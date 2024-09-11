#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <set>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

struct struct_name
{
	std::string Template;
	std::string Name;
};

struct template_args
{
	std::string TemplateArgs;
	std::string ArgumentPass;

    struct Hash
    {
        std::size_t operator()(const template_args& Oth) const
        {
            std::size_t Hash1 = std::hash<std::string>{}(Oth.TemplateArgs);
            std::size_t Hash2 = std::hash<std::string>{}(Oth.ArgumentPass);
            return Hash1 ^ (Hash2 << 1);
        }
    };

    bool operator<(const template_args& other) const
    {
        return (TemplateArgs + ArgumentPass) < (other.TemplateArgs + other.ArgumentPass);
    }
};

template_args ParsedTemplateArgs;
std::string TemplatePrefix;
std::set<std::pair<std::pair<std::string, std::string>, std::pair<bool, template_args>>> TypeArrayNames;
std::set<std::string> EnumNames;
std::vector<struct_name> StructHierarchy;
bool IsIntrospactable = false;
bool WithTemplate = false;

struct meta_struct
{
    char* Name;
    meta_struct* Next;
};
static meta_struct* FirstMetaStruct;

static char*
ReadFile(const char *FileName)
{
    char *Result = 0;
    
    FILE *File = fopen(FileName, "r");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result = (char *)malloc(FileSize + 1);
        fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;
        
        fclose(File);
    }

    return(Result);
}

enum token_type
{
    Token_Unknown,
    
    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_Semicolon,
    Token_Asterisk,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
	Token_OpenAngle,
	Token_CloseAngle,

	Token_Comma,

    Token_String,
    Token_Identifier,

    Token_EndOfStream,
};
struct token
{
    token_type Type;
    
    size_t TextLength;
    char* Text;
};

struct tokenizer
{
    char* At;
};

inline bool
IsEndOfLine(char C)
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));

    return(Result);
}

inline bool
IsWhitespace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\v') ||
                   (C == '\f') ||
                   IsEndOfLine(C));

    return(Result);
}

inline bool
IsAlpha(char C)
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));

    return(Result);
}

inline bool
IsNumber(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));
    
    return(Result);
}

inline bool
TokenEquals(token Token, const char* Match)
{
    const char* At = Match;
    for(int Index = 0;
        Index < Token.TextLength;
        ++Index, ++At)
    {
        if((*At == 0) ||
           (Token.Text[Index] != *At))
        {
            return(false);
        }
        
    }

    bool Result = (*At == 0);
    return(Result);
}

static void
EatAllWhitespace(tokenizer* Tokenizer)
{
    for(;;)
    {
        if(IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '*'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] &&
                  !((Tokenizer->At[0] == '*') &&
                    (Tokenizer->At[1] == '/')))
            {
                ++Tokenizer->At;
            }
            
            if(Tokenizer->At[0] == '*')
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

static token
GetToken(tokenizer* Tokenizer)
{
    EatAllWhitespace(Tokenizer);

    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokenizer->At;
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': {Token.Type = Token_EndOfStream;} break;
           
        case '(': {Token.Type = Token_OpenParen;} break;
        case ')': {Token.Type = Token_CloseParen;} break;
        case '[': {Token.Type = Token_OpenBracket;} break;
        case ']': {Token.Type = Token_CloseBracket;} break;
        case '{': {Token.Type = Token_OpenBrace;} break;
        case '}': {Token.Type = Token_CloseBrace;} break;
        case ';': {Token.Type = Token_Semicolon;} break;
        case '*': {Token.Type = Token_Asterisk;} break;
		case ',': {Token.Type = Token_Comma;} break;
		case '<': {Token.Type = Token_OpenAngle;} break;
		case '>': {Token.Type = Token_CloseAngle;} break;

        case ':':
        {
            if (Tokenizer->At[0] == ':')
            {
                Tokenizer->At++;
                Token.Type = Token_Identifier;
                Token.TextLength = 2;
            }
            else
            {
                Token.Type = Token_Colon;
            }
        } break;
            
        case '"':
        {
            Token.Type = Token_String;

            Token.Text = Tokenizer->At;
            
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"')
            {
                if((Tokenizer->At[0] == '\\') &&
                   Tokenizer->At[1])
                {
                    ++Tokenizer->At;
                }                
                ++Tokenizer->At;
            }

            Token.TextLength = Tokenizer->At - Token.Text;
            if(Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;

        default:
        {
			if (IsAlpha(C))
            {
                Token.Type = Token_Identifier;
                Token.Text = Tokenizer->At - 1;

                while (IsAlpha(Tokenizer->At[0]) || IsNumber(Tokenizer->At[0]) || (Tokenizer->At[0] == '_'))
                {
                    ++Tokenizer->At;
                }

                while (Tokenizer->At[0] == ':' && Tokenizer->At[1] == ':')
                {
                    Tokenizer->At += 2;

                    while (IsAlpha(Tokenizer->At[0]) || IsNumber(Tokenizer->At[0]) || (Tokenizer->At[0] == '_'))
                    {
                        ++Tokenizer->At;
                    }
                }

                Token.TextLength = Tokenizer->At - Token.Text;
            }
            else
            {
                Token.Type = Token_Unknown;
            }
        } break;        
    }

    return(Token);
}

static bool
RequireToken(tokenizer* Tokenizer, token_type DesiredType)
{
    token Token = GetToken(Tokenizer);
    bool Result = (Token.Type == DesiredType);
    return(Result);
}

static template_args 
ParseTemplateArgs(const std::string& Template)
{
	template_args Result = {};
	tokenizer Tokenizer;
	Tokenizer.At = (char*)Template.c_str();

	bool ParsingArgs = false;
	bool ParsingIdentifiers = false;

	while (true)
	{
		token Token = GetToken(&Tokenizer);
		if (Token.Type == Token_EndOfStream)
		{
			break;
		}

		if (Token.Type == Token_OpenAngle)
		{
			Result.ArgumentPass += "<";
			ParsingArgs = true;
			continue;
		}
		else if (Token.Type == Token_CloseAngle)
		{
			Result.ArgumentPass += ">";
			break;
		}

		if (ParsingArgs)
		{
			if (Token.Type == Token_Identifier)
			{
				std::string TokenText(Token.Text, Token.TextLength);

				if (!Result.TemplateArgs.empty())
					Result.TemplateArgs += " ";
				Result.TemplateArgs += TokenText;

				if (ParsingIdentifiers)
				{
					if (Result.ArgumentPass.back() != '<')
						Result.ArgumentPass += ", ";
					Result.ArgumentPass += TokenText;
					ParsingIdentifiers = false;
				}
				else
				{
					ParsingIdentifiers = true;
				}
			}
			else if (Token.Type == Token_Comma)
			{
				ParsingIdentifiers = false;
				Result.TemplateArgs += ", ";
			}
		}
	}

	return Result;
}


static void
ParseIntrospectData(tokenizer* Tokenizer)
{
    for(;;)
    {
        token Token = GetToken(Tokenizer);
        if((Token.Type == Token_CloseParen) ||
           (Token.Type == Token_EndOfStream))
        {
            break;
        }
    }
}

static void
ParseMember(tokenizer* Tokenizer, token StructTypeToken, token MemberTypeToken)
{
    bool Parsing = true;
    bool IsPointer = false;
	bool IsArray = false;
    while(Parsing)
    {
        token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
            case Token_Asterisk:
            {
                IsPointer = true;
            } break;

			case Token_OpenBracket:
			{
				IsArray = true;
				while (true)
				{
					token NextToken = GetToken(Tokenizer);
					if (NextToken.Type == Token_CloseBracket)
					{
						break;
					}
				}
			} break;

			case Token_CloseBracket:
			{
				IsArray = false;
			} break;

            case Token_Identifier:
            {
				if(!IsIntrospactable) break;

				std::string FullMemberTypeName;
				std::string FullStructTypeName;
				for (size_t i = 0; i < StructHierarchy.size(); ++i)
				{
					FullStructTypeName += StructHierarchy[i].Name;

					if (i < StructHierarchy.size() - 1)
					{
						FullStructTypeName += "::";
					}
				}
				std::string MemberTypeName(MemberTypeToken.Text, MemberTypeToken.TextLength);
				size_t pos = 0;
				while ((pos = MemberTypeName.find("::", pos)) != std::string::npos) {
					MemberTypeName.replace(pos, 2, "__");
				}
				FullMemberTypeName += MemberTypeName;

                printf("   {%s, MetaType__%.*s, \"%.*s\", sizeof(%.*s), offsetof(%.*s, %.*s)},\n",
                       IsPointer ? "MetaMemberFlag_IsPointer" : "0",
                       FullMemberTypeName.length(), FullMemberTypeName.c_str(),
                       MemberTypeToken.TextLength, MemberTypeToken.Text,
                       MemberTypeToken.TextLength, MemberTypeToken.Text,
                       FullStructTypeName.length(), FullStructTypeName.c_str(),
                       Token.TextLength, Token.Text);                

				EnumNames.insert("MetaType__" + FullMemberTypeName);
            } break;

            case Token_Semicolon:
            case Token_EndOfStream:
            {

                Parsing = false;
            } break;
        }
    }
}

static void
ParseStruct(tokenizer* Tokenizer)
{
    token NameToken = GetToken(Tokenizer);
	StructHierarchy.push_back({TemplatePrefix, std::string(NameToken.Text, NameToken.TextLength)});

	token LookaheadToken = GetToken(Tokenizer);
    if (LookaheadToken.Type == Token_Colon)
    {
        while (true)
        {
            token NextToken = GetToken(Tokenizer);
            if (NextToken.Type == Token_OpenBrace)
            {
                Tokenizer->At -= NextToken.TextLength;
                break;
            }

            if (NextToken.Type == Token_Identifier)
            {
                NameToken = NextToken;
            }
        }
    }
    else
    {
        Tokenizer->At -= LookaheadToken.TextLength;
    }

    if(RequireToken(Tokenizer, Token_OpenBrace))
    {
		if(IsIntrospactable)
		{
			std::string FullStructName;
			for (size_t i = 0; i < StructHierarchy.size(); ++i)
			{
				FullStructName += StructHierarchy[i].Name;

				if (i < StructHierarchy.size() - 1)
				{
					FullStructName += "__";
				}
			}

			TypeArrayNames.insert({{StructHierarchy[0].Name, "MembersOf__" + FullStructName}, {WithTemplate, ParsedTemplateArgs}});
			printf("member_definition MembersOf__%.*s[] = \n", FullStructName.length(), FullStructName.c_str());
			printf("{\n");
		}

        for(;;)
        {
            token MemberToken = GetToken(Tokenizer);
            if(MemberToken.Type == Token_CloseBrace)
            {
                break;
            }
			else if(TokenEquals(MemberToken, "struct"))
			{
				ParseStruct(Tokenizer);
			}
			else if(TokenEquals(MemberToken, "introspect"))
			{
				ParseIntrospectData(Tokenizer);
				IsIntrospactable = true;
			}
            else
            {
                ParseMember(Tokenizer, NameToken, MemberToken);
            }
        }
		if(IsIntrospactable)
		{
			printf("};\n");
		}

        meta_struct *Meta = (meta_struct *)malloc(sizeof(meta_struct));
        Meta->Name = (char *)malloc(NameToken.TextLength + 1);
        memcpy(Meta->Name, NameToken.Text, NameToken.TextLength);
        Meta->Name[NameToken.TextLength] = 0;
        Meta->Next = FirstMetaStruct;
        FirstMetaStruct = Meta;
    }

	StructHierarchy.pop_back();
	IsIntrospactable = false;
	Tokenizer->At += 2;
}


int main(int ArgCount, char** Args)
{
	namespace fs = std::filesystem;

	for(const auto& Entry : fs::directory_iterator("../src/core/gfx/functions"))
    {
		if(!Entry.is_regular_file()) continue;

		std::string EntryFile = Entry.path().string();
        char* FileContents = ReadFile(EntryFile.c_str());

        tokenizer Tokenizer = {};
        Tokenizer.At = FileContents;

        bool Parsing = true;
        while(Parsing)
        {
            token Token = GetToken(&Tokenizer);
            switch(Token.Type)
            {
                case Token_EndOfStream:
                {
                    Parsing = false;
                } break;

                case Token_Unknown:
                {
                } break;

                case Token_Identifier:
                {                
					if(TokenEquals(Token, "struct"))
					{
						ParseStruct(&Tokenizer);
					}
					else if(TokenEquals(Token, "introspect"))
                    {
						IsIntrospactable = true;
                    }
					else if(TokenEquals(Token, "template"))
					{
						WithTemplate = true;
						Token = GetToken(&Tokenizer);
						TemplatePrefix += "template";

						while (Token.Type != Token_CloseAngle)
						{
							TemplatePrefix += std::string(Token.Text, Token.TextLength) + " ";
							Token = GetToken(&Tokenizer);
						}
						
						TemplatePrefix += ">";
						Token = GetToken(&Tokenizer);

						ParsedTemplateArgs = ParseTemplateArgs(TemplatePrefix);

						if(TokenEquals(Token, "struct"))
						{
							ParseStruct(&Tokenizer);
						}

						TemplatePrefix.clear();
						WithTemplate = false;
					}
                } break;
                        
                default:
                {
                } break;
            }
        }
    }

	for(const auto& TypeArrayName : TypeArrayNames)
	{
		const std::string& TypeName  = TypeArrayName.first.first;
		const std::string& ArrayName = TypeArrayName.first.second;
		const bool& IsWithTemplate   = TypeArrayName.second.first;
		const template_args& Templ   = TypeArrayName.second.second;

		printf("template<%.*s>\nstruct reflect<%.*s>\n{\n\tstatic meta_descriptor* Get()\n\t{\n\t\tstatic meta_descriptor Meta{%.*s, sizeof(%.*s)/sizeof(%.*s[0])};\n\t\treturn &Meta;\n\t}\n};\n", Templ.TemplateArgs.length(), Templ.TemplateArgs.c_str(), (TypeName + Templ.ArgumentPass).length(), (TypeName + Templ.ArgumentPass).c_str(), ArrayName.length(), ArrayName.c_str(), ArrayName.length(), ArrayName.c_str(), ArrayName.length(), ArrayName.c_str());
	}

	printf("enum meta_type\n{\n");
	for (const auto& MemberName : EnumNames) 
	{
		printf("\t%s,\n", MemberName.c_str());
	}
	printf("};\n");
}
