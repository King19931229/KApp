#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/IKDataStream.h"
#include <string>
#include <memory>

struct IKXMLDeclaration;
struct IKXMLAttribute;
struct IKXMLElement;
struct IXMLDocument;

typedef std::shared_ptr<IKXMLDeclaration> IKXMLDeclarationPtr;
typedef std::shared_ptr<IKXMLAttribute> IKXMLAttributePtr;
typedef std::shared_ptr<IKXMLElement> IKXMLElementPtr;
typedef std::shared_ptr<IXMLDocument> IKXMLDocumentPtr;

struct IKXMLDeclaration
{
	virtual ~IKXMLDeclaration() {}
	virtual bool IsEmpty() = 0;
	virtual void SetValue(const char* value) = 0;
	virtual std::string Value() const = 0;
};

struct IKXMLAttribute
{
	virtual ~IKXMLAttribute() {}
	virtual bool IsReadOnly() const = 0;
	virtual bool IsEmpty() const = 0;

	virtual bool BoolValue() const = 0;
	virtual int IntValue() const = 0;
	virtual float FloatValue() const = 0;
	virtual std::string Value() const = 0;

	virtual void SetAttribute(bool value) = 0;
	virtual void SetAttribute(float value) = 0;
	virtual void SetAttribute(double value) = 0;
	virtual void SetAttribute(unsigned int value) = 0;
	virtual void SetAttribute(int value) = 0;
	virtual void SetAttribute(const char* value) = 0;

	virtual IKXMLAttributePtr NextAttribute() const = 0;
};

struct IKXMLElement
{
	virtual ~IKXMLElement() {}

	virtual bool IsEmpty() const = 0;
	virtual bool HasNextSiblingElement() const = 0;

	virtual IKXMLElementPtr NextSiblingElement(const char* name = nullptr) const = 0;
	virtual IKXMLElementPtr FirstChildElement(const char* name = nullptr) const = 0;

	virtual std::string GetValue() const = 0;
	virtual std::string GetText() const = 0;

	virtual IKXMLAttributePtr FirstAttribute() const = 0;
	virtual IKXMLAttributePtr FindAttribute(const char* attribute) const = 0;

	virtual void DeleteAttribute(const char* attribute) = 0;
	virtual void SetAttribute(const char* attribute, bool value) = 0;
	virtual void SetAttribute(const char* attribute, float value) = 0;
	virtual void SetAttribute(const char* attribute, double value) = 0;
	virtual void SetAttribute(const char* attribute, unsigned int value) = 0;
	virtual void SetAttribute(const char* attribute, int value) = 0;
	virtual void SetAttribute(const char* attribute, const char* value) = 0;

	virtual IKXMLElementPtr NewElement(const char* element, bool linkAtOnce = true) = 0;
	virtual void LinkElementToEnd(IKXMLElementPtr& element) = 0;
};

struct IXMLDocument
{
	virtual ~IXMLDocument() {}
	virtual bool SaveFile(const char* fileName) = 0;
	virtual bool ParseFromFile(const char* fileName) = 0;
	virtual bool ParseFromString(const char* text) = 0;
	virtual IKXMLDeclarationPtr NewDeclaration(const char* decl, bool linkAtOnce = true) = 0;
	virtual IKXMLElementPtr NewElement(const char* element, bool linkAtOnce = true) = 0;
	virtual bool HasChildrenElement() const = 0;
	virtual IKXMLElementPtr FirstChildElement(const char* name = nullptr) const = 0;
	virtual void LinkElementToEnd(IKXMLElementPtr& element) = 0;
	virtual void LinkDeclarationToEnd(IKXMLDeclarationPtr& decl) = 0;
};

EXPORT_DLL IKXMLDocumentPtr GetXMLDocument();