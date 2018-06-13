/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: Properties.h 56 2007-05-22 09:05:13Z Pavel Kraynyukhov $
 * 
 **/

#ifndef __PROPERTIES_H__
#define __PROPERTIES_H__

#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <StringTokenizer.h>
#include <Log.h>

namespace utils {

    enum propTypes {
        INT, STRING, BOOLEAN, ERROR
    };

    struct properties {
        propTypes ptype;

        union {
            int *integer;
            std::string *str;
            bool *Bool;
        } value;

        properties() {
            ptype = ERROR;
            value.integer = NULL;
        }
    };
    /**
     * @brief legacy class for properties file parsing. This class is moved from 
     * utils and it is deprecated now.
     * 
     **/
    class Properties {
    public:
        typedef std::map<std::string, properties*> PropertiesMap;
        typedef std::map<std::string, properties*>::iterator PropertiesMapIt;
    private:
        std::map<std::string, properties*> _properties;
        std::string mPropFileName;
        std::ifstream propertiesFile;

    protected:

        void readProperties() {
            char buf[256];
            std::pair<std::string*, properties*> *ppair;
            size_t linenr = 0;

            while (!propertiesFile.eof()) {
                propertiesFile.getline(buf, 256);
                linenr++;
                if ((buf[0] == '#') || (buf[0] == '\n') || (buf[0] == '\0')) continue;
                ppair = parseLine(buf);
                if (ppair != NULL) {
                    _properties[*(ppair->first)] = ppair->second;
                } else {
                    itc::getLog()->error(__FILE__,__LINE__,"Properties parse error at line: %d, in file: %s\n", linenr, mPropFileName.c_str());
                    break;
                }
            }
        }

        std::pair<std::string*, properties*>* parseLine(char* buf) {

            StringTokenizer sT(std::string(buf), std::string("="));

            std::string* tmpstr = NULL;
            std::string* tmpstr2 = NULL;
            properties* tmpProps = new properties();
            std::pair<std::string*, properties*>* tmpPair = NULL;
            bool success = false;

            if (sT.size() == 2) {
                tmpstr = new std::string(sT.nextToken());
                tmpstr2 = new std::string(sT.nextToken());

                if (isalpha(((tmpstr2->c_str())[0])) != 0) {
                    bool booleanTrue = (strncasecmp("true", tmpstr2->c_str(), 4) == 0);
                    bool booleanFalse = (strncasecmp("false", tmpstr2->c_str(), 5) == 0);
                    if (booleanTrue || booleanFalse) {
                        tmpProps->ptype = BOOLEAN;
                        if (booleanTrue)
                            tmpProps->value.Bool = new bool(true);
                        else
                            tmpProps->value.Bool = new bool(false);
                        success = true;

                    } else {
                        tmpProps->ptype = STRING;
                        tmpProps->value.str = tmpstr2;
                        success = true;
                    }
                } else {
                    size_t countDigs = 0;
                    for (size_t i = 0; i < tmpstr2->length(); i++) {
                        if (isdigit(((tmpstr2->c_str())[i])) != 0)
                            countDigs++;
                        else break;
                    }
                    if (countDigs != (tmpstr2->length())) {
                        tmpProps->ptype = STRING;
                        tmpProps->value.str = tmpstr2;
                        success = true;
                    } else {
                        tmpProps->ptype = INT;
                        tmpProps->value.integer = new int(atoi(tmpstr2->c_str()));
                        success = true;
                    }
                }
            }
            if (success) {
                tmpPair = new std::pair<std::string*, properties*>();
                tmpPair->first = tmpstr;
                tmpPair->second = tmpProps;
            }
            return tmpPair;
        }
    public:

        explicit Properties(const std::string& filename) : mPropFileName(filename), propertiesFile(filename.c_str()) {
            readProperties();
        }

        explicit Properties(const char* filename) : mPropFileName(filename), propertiesFile(filename) {
            readProperties();
        }

        bool getPropertyByName(const std::string& props, int* value) {
            std::map<std::string, properties*>::const_iterator CI = _properties.find(props);
            if (CI == _properties.end()) return false;
            if ((value == NULL) || (CI->second->ptype != INT)) return false;
            (*((int*) value)) = *(CI->second->value.integer);
            return true;
        }

        bool getPropertyByName(const std::string& props, bool* value) {
            std::map<std::string, properties*>::const_iterator CI = _properties.find(props);
            if (CI == _properties.end()) return false;
            if ((value == NULL) || (CI->second->ptype != BOOLEAN)) return false;
            (*((bool*)value)) = *(CI->second->value.Bool);
            value = CI->second->value.Bool;
            return true;
        }

        bool getPropertyByName(const std::string& props, std::string* value) {
            std::map<std::string, properties*>::const_iterator CI = _properties.find(props);
            if (CI == _properties.end()) return false;

            if ((value == NULL) || (CI->second->ptype != STRING)) return false;
            (*((std::string*)value)) = *(CI->second->value.str);
            return true;
        }
    };

}

#endif

