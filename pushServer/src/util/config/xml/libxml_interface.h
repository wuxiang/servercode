#ifndef _LIBXML_INTERFACE_H
#define _LIBXML_INTERFACE_H

#include <string>
#include <map>
#include <iostream>
#include <memory>

namespace LibXml {
	class DOMAttributes	{
	public:
		typedef std::map<std::string, std::string> map;
		
		virtual ~DOMAttributes() {}
		
		virtual bool get(const std::string&, std::string&) = 0;
		virtual map toMap() = 0;
	};
	typedef std::auto_ptr<DOMAttributes> DOMAttributesPtr;
		
	class DOMNode {
	public:
		virtual ~DOMNode() {}
		
		virtual std::auto_ptr<DOMNode> getFirstChildNode() = 0;
		virtual std::auto_ptr<DOMNode> getNextSiblingNode() = 0;
		virtual std::auto_ptr<DOMAttributes> getAttributes() = 0;
		virtual std::string getName() = 0;
		virtual std::string getText() = 0;
		virtual const int getType() = 0;
	};
	typedef std::auto_ptr<DOMNode> DOMNodePtr;
		
	class DOMDocument {
	public:
		virtual ~DOMDocument() {}
		
		virtual bool load( std::istream& ) = 0;
		virtual bool load( const std::string& ) = 0;
		virtual bool xml( std::ostream& ) = 0;
		
		virtual std::auto_ptr<DOMNode> getNode( const std::string& ) = 0;
	};
	typedef std::auto_ptr<DOMDocument> DOMDocumentPtr;
};

#endif	/* _LIBXML_INTERFACE_H */
