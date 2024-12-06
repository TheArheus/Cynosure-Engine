#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/AST/RecordLayout.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/SmallString.h>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <memory>
#include <cstdio>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

struct MemberInfo
{
    std::string TypeName;
    std::string BaseTypeName;
    std::string MemberName;
    uint32_t BaseTypeSize;
    size_t ArraySize;
	uint64_t OffsetInBytes;
};

struct ClassInfo
{
    std::string QualifiedName;
    std::string SimpleName;
    std::vector<MemberInfo> Members;
};

class ReflectionVisitor : public RecursiveASTVisitor<ReflectionVisitor>
{
public:
    explicit ReflectionVisitor(ASTContext* Context) : Context(Context) {}

    bool VisitCXXRecordDecl(CXXRecordDecl* Declaration)
    {
        if (!Declaration->isCompleteDefinition())
        {
            return true;
        }

#if 0
		SourceManager& SM = Context->getSourceManager();
		if (!SM.isInMainFile(Declaration->getLocation()))
		{
			return true;
		}
#endif

        bool IsAnnotated = false;
        for (const auto* Attr : Declaration->attrs())
        {
            if (const AnnotateAttr* Annot = dyn_cast<AnnotateAttr>(Attr))
            {
                if (Annot->getAnnotation() == "shader-input")
                {
                    IsAnnotated = true;
                    break;
                }
            }
        }

        if (!IsAnnotated)
        {
            return true;
        }

        ClassInfo classInfo;
        classInfo.QualifiedName = GetQualifiedName(Declaration);
        classInfo.SimpleName = Declaration->getNameAsString();

        for (auto* Member : Declaration->decls())
        {
            if (auto* Field = dyn_cast<FieldDecl>(Member))
            {
                MemberInfo memberInfo;
                memberInfo.MemberName = Field->getNameAsString();
                ExtractMemberInfo(Field, memberInfo);
                classInfo.Members.push_back(memberInfo);

                UniqueTypes.insert(memberInfo.BaseTypeName);
            }
        }

        Classes.push_back(std::move(classInfo));
        return true;
    }

    const std::vector<ClassInfo>& getClasses() const
    {
        return Classes;
    }

    const std::set<std::string>& getUniqueTypes() const
    {
        return UniqueTypes;
    }

private:
    ASTContext* Context;
    std::vector<ClassInfo> Classes;
    std::set<std::string> UniqueTypes;

    void ExtractMemberInfo(FieldDecl* Field, MemberInfo& memberInfo)
    {
        QualType QT = Field->getType();

        if (QT->isConstantArrayType())
        {
            const ConstantArrayType* CAT = Context->getAsConstantArrayType(QT);
            QualType ElementType = CAT->getElementType();

            memberInfo.ArraySize = CAT->getSize().getZExtValue();
            memberInfo.TypeName = GetFullTypeName(QT);              // Type name including array notation
            memberInfo.BaseTypeName = GetFullTypeName(ElementType); // Base type name

            memberInfo.BaseTypeSize = Context->getTypeSizeInChars(ElementType).getQuantity() / 8; // Size in bytes
        }
        else
        {
            memberInfo.ArraySize = 1;
            memberInfo.TypeName = GetFullTypeName(QT);
            memberInfo.BaseTypeName = memberInfo.TypeName;

            memberInfo.BaseTypeSize = Context->getTypeSizeInChars(QT).getQuantity() / 8; // Size in bytes
        }

        memberInfo.MemberName = Field->getNameAsString();

		const ASTRecordLayout& Layout = Context->getASTRecordLayout(Field->getParent());
		uint64_t OffsetInBits = Layout.getFieldOffset(Field->getFieldIndex());
		memberInfo.OffsetInBytes = OffsetInBits / 8;
    }

    std::string GetQualifiedName(const CXXRecordDecl* Declaration)
    {
        std::string Name = Declaration->getQualifiedNameAsString();

        if (const auto* SpecDecl = dyn_cast<ClassTemplateSpecializationDecl>(Declaration))
        {
            const TemplateArgumentList& TemplateArgs = SpecDecl->getTemplateArgs();
            Name += GetTemplateArgsAsString(TemplateArgs);
        }

        return Name;
    }

    std::string GetTemplateArgsAsString(const TemplateArgumentList& TemplateArgs)
    {
        std::string ArgsStr = "<";
        for (unsigned i = 0; i < TemplateArgs.size(); ++i)
        {
            const TemplateArgument& Arg = TemplateArgs[i];
            if (Arg.getKind() == TemplateArgument::Type)
            {
                ArgsStr += Arg.getAsType().getAsString(Context->getPrintingPolicy());
            }
            else if (Arg.getKind() == TemplateArgument::Integral)
            {
                llvm::APSInt Value = Arg.getAsIntegral();
                llvm::SmallString<16> Str;
                Value.toString(Str, 10);
                ArgsStr += Str.str().str();
            }
            if (i + 1 < TemplateArgs.size())
            {
                ArgsStr += ", ";
            }
        }
        ArgsStr += ">";
        return ArgsStr;
    }

    std::string GetFullTypeName(QualType QT)
    {
        if (const TemplateSpecializationType* TST = QT->getAs<TemplateSpecializationType>())
        {
            std::string TypeName = TST->getTemplateName().getAsTemplateDecl()->getNameAsString();
            TypeName += "<";

            const ArrayRef<TemplateArgument>& TemplateArgs = TST->template_arguments();

            for (unsigned i = 0; i < TemplateArgs.size(); ++i)
            {
                const TemplateArgument& Arg = TemplateArgs[i];
                if (Arg.getKind() == TemplateArgument::Type)
                {
                    TypeName += Arg.getAsType().getAsString(Context->getPrintingPolicy());
                }
                else if (Arg.getKind() == TemplateArgument::Integral)
                {
                    llvm::APSInt Value = Arg.getAsIntegral();
                    llvm::SmallString<16> Str;
                    Value.toString(Str, 10);
                    TypeName += Str.str().str();
                }
                if (i + 1 < TemplateArgs.size())
                {
                    TypeName += ", ";
                }
            }
            TypeName += ">";
            return TypeName;
        }
        else if (QT->isConstantArrayType())
        {
            // For arrays, get the type name and append array notation
            const ConstantArrayType* CAT = Context->getAsConstantArrayType(QT);
            QualType ElementType = CAT->getElementType();
            std::string ElementTypeName = GetFullTypeName(ElementType);

            llvm::SmallString<16> Str;
            CAT->getSize().toString(Str, 10, false);
            return ElementTypeName + "[" + Str.str().str() + "]";
        }
        else
        {
            return QT.getAsString(Context->getPrintingPolicy());
        }
    }
};

class ReflectionASTConsumer : public clang::ASTConsumer
{
public:
    explicit ReflectionASTConsumer(ASTContext* Context, std::string OutputFile)
        : Visitor(Context), OutputFileName(std::move(OutputFile)) {}

    void HandleTranslationUnit(ASTContext& Context) override
    {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
        GenerateCode();
    }

private:
    ReflectionVisitor Visitor;
    std::string OutputFileName;

    void GenerateCode()
    {
		std::string TempFileName = OutputFileName + ".tmp";
        std::ofstream OutputFile(TempFileName);

        if (!OutputFile.is_open())
        {
            llvm::errs() << "Failed to open output file: " << OutputFileName << "\n";
            return;
        }

        // Generate meta_type enum
        OutputFile << "enum class meta_type \n{\n";
        OutputFile << "    unknown = 0,\n";
        for (const auto& TypeName : Visitor.getUniqueTypes())
        {
            std::string EnumName = SanitizeName(TypeName);
            OutputFile << "    " << EnumName << ",\n";
        }
        OutputFile << "};\n\n";

        // Define member_definition struct if not defined
        OutputFile << "struct member_definition \n{\n";
        OutputFile << "    uint32_t Flags;\n";
        OutputFile << "    meta_type Type;\n";
        OutputFile << "    const char* Name;\n";
        OutputFile << "    uint32_t Size;\n";
        OutputFile << "    uint32_t Offset;\n";
        OutputFile << "    size_t ArraySize;\n";
        OutputFile << "};\n\n";

        // Define meta_descriptor struct if not defined
        OutputFile << "struct meta_descriptor \n{\n";
        OutputFile << "    member_definition* Data;\n";
        OutputFile << "    size_t Size;\n";
        OutputFile << "};\n\n";

		OutputFile << "template<typename T>\n";
		OutputFile << "struct reflect\n";
		OutputFile << "{\n";
		OutputFile << "    static meta_descriptor* Get()\n";
		OutputFile << "    {\n";
		OutputFile << "        static meta_descriptor Empty{nullptr, 0};\n";
		OutputFile << "        return &Empty;\n";
		OutputFile << "    }\n";
		OutputFile << "};\n\n\n";

        // Generate code for each class
        for (const auto& Class : Visitor.getClasses())
        {
            // Sanitize class name for identifiers
            std::string ArrayName = "MembersOf__" + SanitizeName(Class.QualifiedName);

            OutputFile << "member_definition " << ArrayName << "[] = \n{\n";

            for (const auto& Member : Class.Members)
            {
                OutputFile << "    {0, meta_type::" << SanitizeName(Member.BaseTypeName) << ", \""
                           << Member.MemberName << "\", sizeof(" << Member.BaseTypeName << "), "
#if 1
                           << "offsetof(" << Class.QualifiedName << ", " << Member.MemberName << "), "
#else
                           << Member.OffsetInBytes << ", "
#endif
                           << Member.ArraySize << "},\n";
            }
            OutputFile << "};\n\n";

            // Generate reflect struct specialization
            OutputFile << "template<>\n";
            OutputFile << "struct reflect<" << Class.QualifiedName << ">\n";
            OutputFile << "{\n";
            OutputFile << "    static meta_descriptor* Get()\n";
            OutputFile << "    {\n";
            OutputFile << "        static meta_descriptor Meta{ " << ArrayName
                       << ", sizeof(" << ArrayName << ")/sizeof(" << ArrayName << "[0]) };\n";
            OutputFile << "        return &Meta;\n";
            OutputFile << "    }\n";
            OutputFile << "};\n\n";
        }

        OutputFile.close();

        if (std::remove(OutputFileName.c_str()) != 0)
        {
            if (errno != ENOENT)
            {
                llvm::errs() << "Failed to remove old output file: " << OutputFileName << "\n";
                std::remove(TempFileName.c_str());
                return;
            }
        }

        if (std::rename(TempFileName.c_str(), OutputFileName.c_str()) != 0)
        {
            llvm::errs() << "Failed to rename temporary file to output file.\n";
            std::remove(TempFileName.c_str());
            return;
        }
    }

    std::string SanitizeName(const std::string& Name)
    {
        std::string Result;
        bool LastCharWasUnderscore = false;
        for (char c : Name)
        {
            if (std::isalnum(c) || c == '_')
            {
                Result += c;
                LastCharWasUnderscore = false;
            }
            else if (c == '<' || c == '>' || c == ':' || c == ',' || c == ' ' || c == '[' || c == ']')
            {
                if (!LastCharWasUnderscore && !Result.empty())
                {
                    Result += '_';
                    LastCharWasUnderscore = true;
                }
            }
            else
            {
                if (!LastCharWasUnderscore && !Result.empty())
                {
                    Result += '_';
                    LastCharWasUnderscore = true;
                }
            }
        }

        // Remove trailing underscore if present
        if (!Result.empty() && Result.back() == '_')
        {
            Result.pop_back();
        }

        // Ensure the name doesn't start with an underscore
        if (!Result.empty() && Result.front() == '_')
        {
            Result = Result.substr(1);
        }

        return Result;
    }
};

class ReflectionFrontendAction : public clang::ASTFrontendAction
{
public:
    explicit ReflectionFrontendAction(std::string OutputFile)
        : OutputFileName(std::move(OutputFile)) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override
    {
        return std::make_unique<ReflectionASTConsumer>(&CI.getASTContext(), OutputFileName);
    }

private:
    std::string OutputFileName;
};

class ReflectionFrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
    explicit ReflectionFrontendActionFactory(std::string OutputFileName)
        : OutputFileName(std::move(OutputFileName)) {}

    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<ReflectionFrontendAction>(OutputFileName);
    }

private:
    std::string OutputFileName;
};

static llvm::cl::OptionCategory ReflectionToolCategory("reflection-tool options");

int main(int argc, const char** argv)
{
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ReflectionToolCategory);
    if (!ExpectedParser)
    {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser& OptionsParser = ExpectedParser.get();

    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    std::string OutputFileName = "reflection.gen.cpp";

    auto Factory = std::make_unique<ReflectionFrontendActionFactory>(OutputFileName);

    return Tool.run(Factory.get());
}
