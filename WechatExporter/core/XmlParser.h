//
//  XmlParser.hpp
//  WechatExporter
//
//  Created by Matthew on 2020/11/12.
//  Copyright © 2020 Matthew. All rights reserved.
//

#ifndef XmlParser_h
#define XmlParser_h

#include <cstdio>
#include <string>
#include <map>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>

class XmlParser;

class XmlParser
{
public:
    XmlParser(const std::string& xml, bool noError = false);
    ~XmlParser();
    bool parseNodeValue(const std::string& xpath, std::string& value) const;
    bool parseNodesValue(const std::string& xpath, std::map<std::string, std::string>& values) const;  // e.g.: /path1/path2/*
    bool parseChildNodesValue(xmlNodePtr parentNode, const std::string& xpath, std::map<std::string, std::string>& values) const;  // e.g.: /path1/path2/*
    bool parseAttributeValue(const std::string& xpath, const std::string& attributeName, std::string& value) const;
    bool parseAttributesValue(const std::string& xpath, std::map<std::string, std::string>& attributes) const;
    
    static xmlNodePtr getChildNode(xmlNodePtr node, const std::string& childName);
    static xmlNodePtr getNextNodeSibling(xmlNodePtr node);
    static std::string getNodeInnerText(xmlNodePtr node);
    static std::string getNodeInnerXml(xmlNodePtr node);
    static std::string getNodeOuterXml(xmlNodePtr node);
    static bool getChildNodeContent(xmlNodePtr node, const std::string& childName, std::string& value);
    static bool getNodeAttributeValue(xmlNodePtr node, const std::string& attributeName, std::string& value);
    
    class XPathEnumerator
    {
    public:
        XPathEnumerator(const XmlParser& xmlParser, xmlNodePtr curNode, const std::string& xpath) : m_xPathObj(NULL), m_numberOfNodes(0), m_cursor(-1)
        {
            m_xPathObj = xmlXPathNodeEval(curNode, BAD_CAST(xpath.c_str()), xmlParser.m_xpathCtx);
            if (NULL != m_xPathObj && NULL != m_xPathObj->nodesetval)
            {
                m_numberOfNodes = m_xPathObj->nodesetval->nodeNr;
            }
        }
        
        XPathEnumerator(const XmlParser& xmlParser, const std::string& xpath) : m_xPathObj(NULL), m_numberOfNodes(0), m_cursor(-1)
        {
            m_xPathObj = xmlXPathEvalExpression(BAD_CAST(xpath.c_str()), xmlParser.m_xpathCtx);
            if (NULL != m_xPathObj && NULL != m_xPathObj->nodesetval)
            {
                m_numberOfNodes = m_xPathObj->nodesetval->nodeNr;
            }
        }
        
        bool isInvalid() const
        {
            return NULL == m_xPathObj;
        }
        
        bool hasNext() const
        {
            return m_cursor < (m_numberOfNodes - 1);
        }
        
        xmlNodePtr nextNode()
        {
            return m_xPathObj->nodesetval->nodeTab[++m_cursor];
        }
        
        ~XPathEnumerator()
        {
            if (NULL != m_xPathObj)
            {
                xmlXPathFreeObject(m_xPathObj);
            }
        }
        
    private:
        xmlXPathObjectPtr m_xPathObj;
        int m_numberOfNodes;
        mutable int m_cursor;
    };
    
public:
    template <class TNodeHandler>
    bool parseWithHandler(const std::string& xpath, TNodeHandler& handler) const;
    xmlXPathObjectPtr evalXPathOnNode(xmlNodePtr node, const std::string& xpath);
    
    bool dumpToFile(const std::string& outputPath);
    
private:
    xmlDocPtr m_doc;
    xmlXPathContextPtr m_xpathCtx;
};

inline std::string XmlParser::getNodeInnerText(xmlNodePtr node)
{
    if (NULL == node->children)
    {
        return "";
    }
    const char* content = reinterpret_cast<const char*>(XML_GET_CONTENT(node->children));
    return NULL == content ? "" : std::string(content);
}

inline std::string XmlParser::getNodeInnerXml(xmlNodePtr node)
{
    xmlChar* content = xmlNodeGetContent(node);
    // const char* szContent = reinterpret_cast<const char*>(content);
    std::string xml = (NULL == content) ? "" : reinterpret_cast<const char*>(content);
    xmlFree(content);
    return xml;
}

inline std::string XmlParser::getNodeOuterXml(xmlNodePtr node)
{
    xmlBufferPtr buffer = xmlBufferCreate();
#ifndef NDEBUG
    int size = xmlNodeDump(buffer, node->doc, node, 0, 1);
#else
    int size = xmlNodeDump(buffer, node->doc, node, 0, 0);  // no format for release
#endif
    
    // const char* content = reinterpret_cast<const char*>(XML_GET_CONTENT(node->children));
    std::string xml;
    if (size > 0 && NULL != buffer->content)
    {
        xml.assign(reinterpret_cast<const char*>(buffer->content), size);
    }
    xmlBufferFree(buffer);
    return xml;
}

template <class TNodeHandler>
bool XmlParser::parseWithHandler(const std::string& xpath, TNodeHandler& handler) const
{
    bool result = false;
    if (m_doc == NULL || m_xpathCtx == NULL)
    {
        return false;
    }
    
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST(xpath.c_str()), m_xpathCtx);
    if (xpathObj != NULL)
    {
        xmlNodeSetPtr xpathNodes = xpathObj->nodesetval;
        if ((xpathNodes) && (xpathNodes->nodeNr > 0))
        {
            if (handler(xpathNodes))
            {
                result = true;
            }
        }
        
        xmlXPathFreeObject(xpathObj);
    }

    return result;
}

#endif /* XmlParser_h */
