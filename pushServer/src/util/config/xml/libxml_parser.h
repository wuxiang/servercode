#ifndef _LIBXML_PARSER_H
#define _LIBXML_PARSER_H

#include "libxml_interface.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

namespace LibXml {
	class LIBXML_DOMAttributes : public DOMAttributes {
	public:
		LIBXML_DOMAttributes(xmlNodePtr pNode)
			: m_pNode(pNode) {}
	
		bool get( const std::string&, std::string& );
		DOMAttributes::map toMap();
	
	private:
		xmlNodePtr m_pNode;
	};
	
	class LIBXML_DOMNode : public DOMNode {
	public:
		LIBXML_DOMNode( xmlNodePtr pNode )
			: m_pNode(pNode) {}
		~LIBXML_DOMNode() {}
	
	DOMNodePtr getFirstChildNode();
	DOMNodePtr getNextSiblingNode();
	DOMAttributesPtr getAttributes();
	std::string getName();
	std::string getText();
	const int getType();
	
	private:
		xmlNodePtr m_pNode;
	};
	
	class LIBXML_DOMDocument : public DOMDocument
	{
	public:
		LIBXML_DOMDocument() throw( const char* );
		~LIBXML_DOMDocument();
	
		bool load( std::istream& );
		bool load( const std::string& );
		bool xml( std::ostream& );
	
		DOMNodePtr getNode( const std::string& );
	
	private:
		xmlDocPtr m_pDoc;
	};
};


#endif	/* _LIBXML_PARSER_H */

