#include <QDebug>
#include <QDateTime>
#include <QString>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonParseError>

#include <Array>


#include "core_dll_global.h"
#include "core_dll.h"


#include <iostream>
#include <fstream>
#include <sstream>

#include <QtNetwork>

#include <windows.h>
#include <tlhelp32.h>

#include "RegulaVIZOCR.h"

#include "unzip.h"


#define VERSION_STR "Beta.1.0.65"

COREDLL_NAME_SPACE_START

#define NEED_LOG_FLAG 0
#define DETAIL_INFO_LOG 0
#define DETAIL_ID_INFO_LOG 0
#define DETAIL_NAME_INFO_LOG 0


QString str2qstr(const std::string str)
{
    //return QString::fromLocal8Bit(str.data());
    return QString::fromUtf8(str.data());
}

std::string qstr2str(const QString qstr)
{
    QByteArray cdata = qstr.toUtf8();
    return std::string(cdata);
}

#define JSON_M(token_str) \
JSonValue_t = ParamObj.value(token_str);  \
res_map[token_str] = JSonValue_t.toString();

QMap<QString, QString> ParseCfgJsonFile(QString str)
{
    QMap<QString, QString> res_map;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError || doc.isNull()) {
        qWarning() << "Json format is wrong." << jsonError.error;
        return res_map;
    }
    QJsonObject rootObj = doc.object();

    QJsonValue ParamValue = rootObj.value("SettingParam");
    if (ParamValue.type() == QJsonValue::Object) {
        QJsonObject ParamObj = ParamValue.toObject();
        QJsonValue JSonValue_t;

        JSON_M("monitor_path")
        JSON_M("monitor_file_name_0")
        JSON_M("monitor_file_name_1")
    }
    return res_map;
}

QMap<QString, QString> ParseNonChangedCfgJsonFile(QString str)
{
    QMap<QString, QString> res_map;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError || doc.isNull()) {
        qWarning() << "Json format is wrong." << jsonError.error;
        return res_map;
    }
    QJsonObject rootObj = doc.object();

    QJsonValue ParamValue = rootObj.value("SettingParam");
    if (ParamValue.type() == QJsonValue::Object) {
        QJsonObject ParamObj = ParamValue.toObject();
        QJsonValue JSonValue_t;

        QStringList key_list;
        QJsonObject::Iterator it;

        for(it = ParamObj.begin(); it != ParamObj.end(); it++)
        {
            QString keyString = it.key();
            key_list.append(keyString);
        }

        foreach (auto key_str, key_list) {
            //qDebug() << __FUNCTION__ << __LINE__ << QString("key %1").arg(key_str);
            JSON_M(key_str)
        }

    }
    return res_map;
}

QString GetLocalVersion()
{
    return QString(VERSION_STR);
}


QString GetLocalIP()
{
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    QString ip;
    QStringList ip_List;
    foreach(QHostAddress address, info.addresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            ip_List.append(address.toString());
        }
    }

    if(ip_List.length() == 1)
    {
        ip = ip_List[0];
    }
    else
    {
        foreach (auto ip_s, ip_List) {
            if(!ip_s.contains("192")){
                ip = ip_s;
            }
        }
        if(ip.length() == 0){
            ip = ip_List[ip_List.length() - 1];
        }
    }
    return ip;
}


const QStringList getDirListUnderDir(const QString &dirPath)
{
    QStringList fileList;
    QDir dir(dirPath);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Dirs);
    foreach (auto fileInfo, fileInfoList) {
        if(fileInfo.isDir())
        {
            fileList.append(fileInfo.absoluteFilePath());
        }
    }
    return fileList;
}


const QStringList getFileListUnderDir(const QString &dirPath)
{
    QStringList fileList;
    QDir dir(dirPath);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Dirs);
    foreach (auto fileInfo, fileInfoList) {
        //if(fileInfo.isDir())
        //{
        //    getFileListUnderDir(fileInfo.absoluteFilePath());
        //}

        if(fileInfo.isFile())
        {
            qDebug() << __FUNCTION__ << __LINE__ << "  : " << fileInfo.absoluteFilePath();
        }
    }
    return fileList;
}

bool SearchMatchedFolder(QString moniter_dir, long base_time_stamp, 
                         QString &matched_moniter_dir_str, 
                         long &matched_moniter_dir_time_stamp,
                         QList<QString> &matched_folder_list)
{
    bool search_flag = false;

    qDebug() << __FUNCTION__ << __LINE__ << "Dir : " << moniter_dir;
    qDebug() << __FUNCTION__ << __LINE__ << "BaseTimeStamp : " << base_time_stamp;

    //QList<QString>  matched_folder_list;

    QStringList dir_string_list = getDirListUnderDir(moniter_dir);
    long dec_max = 0;
    //QString matched_moniter_dir_str;
    foreach (auto dir_str, dir_string_list) {
        bool folder_match_flag = false;
        QString year_str, month_str, day_str, time_stamp_str;
        //QRegExp rx.setPattern("^([^\t]+)\t([^\t]+)\t([^\t]+)$");
        QRegExp rx("_(20[0-9][0-9])_([0-9]{1,2})_([0-9]{1,2})_([3456789][0-9]{7,8})$");
        if (rx.indexIn(dir_str) != -1) {
            year_str = rx.cap(1);
            month_str = rx.cap(2);
            day_str = rx.cap(3);
            time_stamp_str = rx.cap(4);
            folder_match_flag = true;
        }
        if(folder_match_flag)
        {
            bool ok;
            long time_stamp = time_stamp_str.toLong(&ok, 10); 
            if(ok)
            {
                if(time_stamp > base_time_stamp)
                {
                    matched_folder_list.append(dir_str);
                    if(time_stamp >= dec_max)
                    {
                        matched_moniter_dir_str = dir_str;
                        dec_max = time_stamp;
                        search_flag = true;
                        matched_moniter_dir_time_stamp = time_stamp;

                        qDebug() << __FUNCTION__ << __LINE__ << "SearchFlag : " << search_flag;
                        qDebug() << __FUNCTION__ << __LINE__ << "MatchedTimeStamp : " << matched_moniter_dir_time_stamp;
                    }
                }
            }
        }
    }
    return search_flag;
}

QString SaveCfgJsonFile(QMap<QString, QString> json_map)
{
    QJsonDocument jdoc;
    QJsonObject obj;
    

    QJsonObject obj_t;
    QMap<QString, QString>::const_iterator iter = json_map.begin();
    while (iter != json_map.end())
    {
        QJsonObject Member;
        obj_t[iter.key()] = iter.value();
        iter++;
    }

    obj["SettingParam"] = obj_t;
    jdoc.setObject(obj);


    QString content(jdoc.toJson(QJsonDocument::Indented));
    return content;
}


#define PARSE_XML_ELEMENT(xml_token)\
JSonValue_t = ParamObj.value(token_map[xml_token]);\
res_map[xml_token] = JSonValue_t.toString();

QMap<QString, QString> ParseXML(QString str, QMap<QString, QString> &token_map)
{
    QMap<QString, QString> res_map;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError || doc.isNull()) {
        qDebug() << "Json format is wrong." << jsonError.error;
        return res_map;
    }
    QJsonObject rootObj = doc.object();

    QJsonValue ParamValue = rootObj.value("Param");
    if (ParamValue.type() == QJsonValue::Object) {
        QJsonObject ParamObj = ParamValue.toObject();
        QJsonValue JSonValue_t;

        PARSE_XML_ELEMENT("xml_token_for_country")
        PARSE_XML_ELEMENT("xml_token_for_id")
        PARSE_XML_ELEMENT("xml_token_for_birthday")
        PARSE_XML_ELEMENT("xml_token_for_name")
        PARSE_XML_ELEMENT("xml_token_for_gender")
        PARSE_XML_ELEMENT("xml_token_for_type")

        PARSE_XML_ELEMENT("xml_post_token_for_issuing_state_Code")
        PARSE_XML_ELEMENT("xml_post_token_for_id")
        PARSE_XML_ELEMENT("xml_post_token_for_date_of_expiry")
        PARSE_XML_ELEMENT("xml_post_token_for_date_of_issue")
        PARSE_XML_ELEMENT("xml_post_token_for_date_of_birth")
        PARSE_XML_ELEMENT("xml_post_token_for_blace_of_birth_1")
        PARSE_XML_ELEMENT("xml_post_token_for_place_of_birth")
        PARSE_XML_ELEMENT("xml_post_token_for_surname")
        PARSE_XML_ELEMENT("xml_post_token_for_given_names_1")
        PARSE_XML_ELEMENT("xml_post_token_for_given_names")
        PARSE_XML_ELEMENT("xml_post_token_for_nationality")
        PARSE_XML_ELEMENT("xml_post_token_for_gender")
        PARSE_XML_ELEMENT("xml_post_token_for_authority_1")
        PARSE_XML_ELEMENT("xml_post_token_for_authority")
        PARSE_XML_ELEMENT("xml_post_token_for_name_1")
        PARSE_XML_ELEMENT("xml_post_token_for_name")
        PARSE_XML_ELEMENT("xml_post_token_for_nationality_code")
        PARSE_XML_ELEMENT("xml_post_token_for_mrz_type")
        PARSE_XML_ELEMENT("xml_post_token_for_issuing_state_name")
        //PARSE_XML_ELEMENT("xml_post_token_for_mrz_string")
        PARSE_XML_ELEMENT("xml_post_token_for_id_checkdigit")
        PARSE_XML_ELEMENT("xml_post_token_for_birthday_checkdigit")
        PARSE_XML_ELEMENT("xml_post_token_for_date_of_expiry_checkdigit")
        PARSE_XML_ELEMENT("xml_post_token_for_final_checkdigit")
        PARSE_XML_ELEMENT("xml_post_token_for_age")
        PARSE_XML_ELEMENT("xml_post_token_for_month_to_expiry")
        PARSE_XML_ELEMENT("xml_post_token_for_age_at_issue")
        PARSE_XML_ELEMENT("xml_post_token_for_permit_Date_of_issue")
        PARSE_XML_ELEMENT("xml_post_token_for_person_number")

        PARSE_XML_ELEMENT("xml_post_token_for_document_type")

        JSonValue_t = ParamObj.value(token_map["xml_post_token_for_mrz_string"]);

        QByteArray cdata_t = JSonValue_t.toString().toUtf8();
        QString tmp_t;
        tmp_t.prepend(cdata_t);
        res_map["xml_post_token_for_mrz_string"] = tmp_t;

    }
    
    /*
    QMap<QString, QString>::iterator iter = res_map.begin();
    while (iter != res_map.end())
    {
        iter++;
    }
    */

    return res_map;
}


QString SaveJsonFile(QString token_name, QMap<QString, QString> json_map)
{
    QJsonDocument jdoc;
    QJsonObject obj;
    

    QJsonObject obj_t;
    QMap<QString, QString>::iterator iter = json_map.begin();
    while (iter != json_map.end())
    {
        QJsonObject Member;
        obj_t[iter.key()] = iter.value();
        iter++;
    }

    obj[token_name] = obj_t;
    jdoc.setObject(obj);


    QString content(jdoc.toJson(QJsonDocument::Indented));
    return content;
}


QString SaveJsonFile_1(QString token_name, QMap<QString, QVariant> json_map)
{
    QJsonDocument jdoc;
    QJsonObject obj;
    
    QJsonObject obj_t;
    QMap<QString, QVariant>::iterator iter = json_map.begin();
    while (iter != json_map.end())
    {
        if(iter.value().type() == QVariant::Map)
        {
            //iter.value().toMap()
            QJsonObject obj_t_inner;
            QMap<QString, QVariant> tmp_inner = iter.value().toMap();
            QMap<QString, QVariant>::iterator iter_inner = tmp_inner.begin();
            while (iter_inner != tmp_inner.end()){
                if(iter_inner.value().type() == QVariant::String)
                {
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("'%1':'%2'").arg(iter_inner.key()).arg(iter_inner.value().toString());
                    obj_t_inner[iter_inner.key()] = iter_inner.value().toString();
                }
                
                iter_inner++;
            }
            obj_t[iter.key()] = obj_t_inner;
        }
        else if(iter.value().type() == QVariant::String)
        {
            qDebug() << __FUNCTION__ << __LINE__ <<  QString("'%1':'%2'").arg(iter.key()).arg(iter.value().toString());
            obj_t[iter.key()] = iter.value().toString();
        }
        
        iter++;
    }

    obj[token_name] = obj_t;
    jdoc.setObject(obj);


    QString content(jdoc.toJson(QJsonDocument::Indented));
    return content;
}


void SaveStringToFile(QString file_str, const QString content)
{
    QFile file(file_str);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(content.toUtf8());
    file.close();
}

QString SaveJsonFile_0(QString token_name, QString key_pattern, QMap<QString, QString> content_map)
{
    QJsonDocument jdoc;
    QJsonObject obj;
    int pos = 0;
    
    QRegExp reg_key_pattern(key_pattern);

    QJsonObject obj_t;
    QMap<QString, QString>::iterator iter = content_map.begin();
    while (iter != content_map.end())
    {
        QJsonObject Member;
        QString key_val = iter.key();
        pos = reg_key_pattern.indexIn(key_val, 0);

        //qDebug() << __FUNCTION__ << __LINE__ <<  QString("'%1':'%2' %3").arg(key_val).arg(iter.value()).arg(pos);

        if(pos != -1)
        {
            obj_t[key_val] = iter.value();
        }
        iter++;
    }

    obj[token_name] = obj_t;
    jdoc.setObject(obj);


    QString content(jdoc.toJson(QJsonDocument::Indented));
    return content;
}

//Unicode code to character which will be replaced.
QMap<unsigned int, QString> special_unicode_transform_table_0;
QMap<unsigned int, QString> special_unicode_transform_table_1;
QMap<unsigned int, QString> special_unicode_transform_table_2;


QList<QString> name_abbreviate_list;
QList<QString> name_abbreviate_for_aut_list;



QList<QString> similar_letter_list;

QList<QString> arab_country_area_list;


#define AUT_STR_COMPARE_FLAG 1
#define AUT_STR_REG_MATCH_FLAG 2

QMap<QString, unsigned int> name_abbreviate_for_aut_map;

void InitSpecialTable()
{
    // Excel table 0
    special_unicode_transform_table_0[0x00C0] = "A";
    special_unicode_transform_table_0[0x00C1] = "A";
    special_unicode_transform_table_0[0x00C2] = "A";
    special_unicode_transform_table_0[0x00C3] = "A";
    special_unicode_transform_table_0[0x00C4] = "AE";  special_unicode_transform_table_1[0x00C4] = "A";
    special_unicode_transform_table_0[0x00C5] = "AA";  special_unicode_transform_table_1[0x00C5] = "A";
    special_unicode_transform_table_0[0x00C6] = "AE";
    special_unicode_transform_table_0[0x00C7] = "C";
    special_unicode_transform_table_0[0x00C8] = "E";
    special_unicode_transform_table_0[0x00C9] = "E";
    special_unicode_transform_table_0[0x00CA] = "E";
    special_unicode_transform_table_0[0x00CB] = "E";
    special_unicode_transform_table_0[0x00CC] = "I";
    special_unicode_transform_table_0[0x00CD] = "I";
    special_unicode_transform_table_0[0x00CE] = "I";
    special_unicode_transform_table_0[0x00CF] = "I";
    special_unicode_transform_table_0[0x00D0] = "D";
    special_unicode_transform_table_0[0x00D1] = "N";  special_unicode_transform_table_1[0x00D1] = "NXX";
    special_unicode_transform_table_0[0x00D2] = "O";
    special_unicode_transform_table_0[0x00D3] = "O";
    special_unicode_transform_table_0[0x00D4] = "O";
    special_unicode_transform_table_0[0x00D5] = "O";
    special_unicode_transform_table_0[0x00D6] = "OE";  special_unicode_transform_table_1[0x00D6] = "O";
    special_unicode_transform_table_0[0x00D8] = "OE";
    special_unicode_transform_table_0[0x00D9] = "U";
    special_unicode_transform_table_0[0x00DA] = "U";
    special_unicode_transform_table_0[0x00DB] = "U";
    special_unicode_transform_table_0[0x00DC] = "UE";  special_unicode_transform_table_1[0x00DC] = "UXX";  special_unicode_transform_table_2[0x00DC] = "U";
    special_unicode_transform_table_0[0x00DD] = "Y";
    special_unicode_transform_table_0[0x00DE] = "TH";
    special_unicode_transform_table_0[0x0100] = "A";
    special_unicode_transform_table_0[0x0102] = "A";
    special_unicode_transform_table_0[0x0104] = "A";
    special_unicode_transform_table_0[0x0106] = "C";
    special_unicode_transform_table_0[0x0108] = "C";
    special_unicode_transform_table_0[0x010A] = "C";
    special_unicode_transform_table_0[0x010C] = "C";
    special_unicode_transform_table_0[0x010E] = "D";
    special_unicode_transform_table_0[0x0110] = "D";
    special_unicode_transform_table_0[0x0112] = "E";
    special_unicode_transform_table_0[0x0114] = "E";
    special_unicode_transform_table_0[0x0116] = "E";
    special_unicode_transform_table_0[0x0118] = "E";
    special_unicode_transform_table_0[0x011A] = "E";
    special_unicode_transform_table_0[0x011C] = "G";
    special_unicode_transform_table_0[0x011E] = "G";
    special_unicode_transform_table_0[0x0120] = "G";
    special_unicode_transform_table_0[0x0122] = "G";
    special_unicode_transform_table_0[0x0124] = "H";
    special_unicode_transform_table_0[0x0126] = "H";
    special_unicode_transform_table_0[0x0128] = "I";
    special_unicode_transform_table_0[0x012A] = "I";
    special_unicode_transform_table_0[0x012C] = "I";
    special_unicode_transform_table_0[0x012E] = "I";
    special_unicode_transform_table_0[0x0130] = "I";
    special_unicode_transform_table_0[0x0131] = "I";
    special_unicode_transform_table_0[0x0132] = "IJ";
    special_unicode_transform_table_0[0x0134] = "J";
    special_unicode_transform_table_0[0x0136] = "K";
    special_unicode_transform_table_0[0x0139] = "L";
    special_unicode_transform_table_0[0x013B] = "L";
    special_unicode_transform_table_0[0x013D] = "L";
    special_unicode_transform_table_0[0x013F] = "L";
    special_unicode_transform_table_0[0x0141] = "L";
    special_unicode_transform_table_0[0x0143] = "N";
    special_unicode_transform_table_0[0x0145] = "N";
    special_unicode_transform_table_0[0x0147] = "N";
    special_unicode_transform_table_0[0x014A] = "N";
    special_unicode_transform_table_0[0x014C] = "O";
    special_unicode_transform_table_0[0x014E] = "O";
    special_unicode_transform_table_0[0x0150] = "O";
    special_unicode_transform_table_0[0x0152] = "OE";
    special_unicode_transform_table_0[0x0154] = "R";
    special_unicode_transform_table_0[0x0156] = "R";
    special_unicode_transform_table_0[0x0158] = "R";
    special_unicode_transform_table_0[0x015A] = "S";
    special_unicode_transform_table_0[0x015C] = "S";
    special_unicode_transform_table_0[0x015E] = "S";
    special_unicode_transform_table_0[0x0160] = "S";
    special_unicode_transform_table_0[0x0162] = "T";
    special_unicode_transform_table_0[0x0164] = "T";
    special_unicode_transform_table_0[0x0166] = "T";
    special_unicode_transform_table_0[0x0168] = "U";
    special_unicode_transform_table_0[0x016A] = "U";
    special_unicode_transform_table_0[0x016C] = "U";
    special_unicode_transform_table_0[0x016E] = "U";
    special_unicode_transform_table_0[0x0170] = "U";
    special_unicode_transform_table_0[0x0172] = "U";
    special_unicode_transform_table_0[0x0174] = "W";
    special_unicode_transform_table_0[0x0176] = "Y";
    special_unicode_transform_table_0[0x0178] = "Y";
    special_unicode_transform_table_0[0x0179] = "Z";
    special_unicode_transform_table_0[0x017B] = "Z";
    special_unicode_transform_table_0[0x017D] = "Z";
    special_unicode_transform_table_0[0x00DF] = "SS";

    // Excel table 1
    //special_unicode_transform_table_0[0x0401] = "E（例外：白俄罗斯语= IO）
    special_unicode_transform_table_0[0x0402] = "D";
    //special_unicode_transform_table_0[0x0404] = "IE（例外：乌克兰语首字，则=YE）
    special_unicode_transform_table_0[0x0405] = "DZ";
    special_unicode_transform_table_0[0x0406] = "I";
    //special_unicode_transform_table_0[0x0407] = "I（例外：乌克兰语首字，则=YI）
    special_unicode_transform_table_0[0x0408] = "J";
    special_unicode_transform_table_0[0x0409] = "LJ";
    special_unicode_transform_table_0[0x040A] = "NJ";
    //special_unicode_transform_table_0[0x040C] = "K（例外：在塞尔维亚语和前南斯拉夫马其顿共和国所讲语言= KJ）
    special_unicode_transform_table_0[0x040E] = "U";
    //special_unicode_transform_table_0[0x040F] = "DZ（例外：在塞尔维亚语和前南斯拉夫马其顿共和国所讲语言= DJ）
    special_unicode_transform_table_0[0x0410] = "A";
    special_unicode_transform_table_0[0x0411] = "B";
    special_unicode_transform_table_0[0x0412] = "V";
    //special_unicode_transform_table_0[0x0413] = "G（例外：白俄罗斯语、塞尔维亚语和乌克兰语=H）
    special_unicode_transform_table_0[0x0414] = "D";
    special_unicode_transform_table_0[0x0415] = "E";
    //special_unicode_transform_table_0[0x0416] = "ZH（例外：塞尔维亚语=Z）
    special_unicode_transform_table_0[0x0417] = "Z";
    //special_unicode_transform_table_0[0x0418] = "I（例外：乌克兰语=Y）
    //special_unicode_transform_table_0[0x0419] = "I（例外：乌克兰语首字，则=Y）
    special_unicode_transform_table_0[0x041A] = "K";
    special_unicode_transform_table_0[0x041B] = "L";
    special_unicode_transform_table_0[0x041C] = "M";
    special_unicode_transform_table_0[0x041D] = "N";
    special_unicode_transform_table_0[0x041E] = "O";
    special_unicode_transform_table_0[0x041F] = "P";
    special_unicode_transform_table_0[0x0420] = "R";
    special_unicode_transform_table_0[0x0421] = "S";
    special_unicode_transform_table_0[0x0422] = "T";
    special_unicode_transform_table_0[0x0423] = "U";
    special_unicode_transform_table_0[0x0424] = "F";

    //special_unicode_transform_table_0[0x0425] = "KH（例外：塞尔维亚语和前南斯拉夫 马其顿共和国语=H）
    //special_unicode_transform_table_0[0x0426] = "TS（例外：塞尔维亚语和前南斯拉夫 马其顿共和国语=C）
    //special_unicode_transform_table_0[0x0427] = "CH（例外：塞尔维亚语=C）
    //special_unicode_transform_table_0[0x0428] = "SH（例外：塞尔维亚语=S）
    //special_unicode_transform_table_0[0x0429] = "SHCH（例外：保加利亚语=SHT）
    special_unicode_transform_table_0[0x042A] = "IE";
    special_unicode_transform_table_0[0x042B] = "Y";
    special_unicode_transform_table_0[0x042D] = "E";
    //special_unicode_transform_table_0[0x042E] = "IU（例外：乌克兰语首字，则=YU）
    //special_unicode_transform_table_0[0x042F] = "IA（例外：乌克兰语首字，则=YA）
    special_unicode_transform_table_0[0x046A] = "U";
    special_unicode_transform_table_0[0x0474] = "Y";
    special_unicode_transform_table_0[0x0490] = "G";
    //special_unicode_transform_table_0[0x0492] = "G（例外：在前南斯拉夫马其顿共和国0492 F所讲语言=GJ）
    special_unicode_transform_table_0[0x04BA] = "C";


    similar_letter_list.append(QString("[EF]"));
    similar_letter_list.append(QString("[ITL]"));
    similar_letter_list.append(QString("[MNWVY]"));
    similar_letter_list.append(QString("[CGOQ]"));
    similar_letter_list.append(QString("[CPR]"));

    // arab contry
    arab_country_area_list.append(QString("SAU"));
    arab_country_area_list.append(QString("ARE"));
    arab_country_area_list.append(QString("YEM"));
    arab_country_area_list.append(QString("IRQ"));
    arab_country_area_list.append(QString("IRN"));
    arab_country_area_list.append(QString("JOR"));
    arab_country_area_list.append(QString("LBN"));
    arab_country_area_list.append(QString("SYR"));
    arab_country_area_list.append(QString("PSE"));
    arab_country_area_list.append(QString("BHR"));
    arab_country_area_list.append(QString("QAT"));
    arab_country_area_list.append(QString("KWT"));
    arab_country_area_list.append(QString("OMN"));

    arab_country_area_list.append(QString("AFG"));
    arab_country_area_list.append(QString("ISR"));



    name_abbreviate_list.append(QString("MR."));
    name_abbreviate_list.append(QString("MIR."));
    name_abbreviate_list.append(QString("MISS."));
    name_abbreviate_list.append(QString("MRS."));
    name_abbreviate_list.append(QString("MS."));
    name_abbreviate_list.append(QString("MASTER."));

    name_abbreviate_list.append(QString("MR"));
    name_abbreviate_list.append(QString("MIR"));
    name_abbreviate_list.append(QString("MISS"));
    name_abbreviate_list.append(QString("MRS"));
    name_abbreviate_list.append(QString("MS"));
    name_abbreviate_list.append(QString("MASTER"));

    name_abbreviate_list.append(QString("S/O"));
    name_abbreviate_list.append(QString("D/O"));
    name_abbreviate_list.append(QString("MD."));
    name_abbreviate_list.append(QString("MD"));

    name_abbreviate_list.append(QString("DR."));
    name_abbreviate_list.append(QString("DIPL."));
    name_abbreviate_list.append(QString("ING."));
    name_abbreviate_list.append(QString("MAG."));

    name_abbreviate_list.append(QString("DR"));
    name_abbreviate_list.append(QString("DIPL"));
    name_abbreviate_list.append(QString("ING"));
    name_abbreviate_list.append(QString("MAG"));


    name_abbreviate_list.append(QString("(FH)"));
    name_abbreviate_list.append(QString("H.E."));

    name_abbreviate_list.append(QString("EP."));
    name_abbreviate_list.append(QString("VVE."));
    name_abbreviate_list.append(QString("GEB."));
    name_abbreviate_list.append(QString("G/V"));
    name_abbreviate_list.append(QString("E/V"));
    name_abbreviate_list.append(QString("HUSBAND.OF"));
    name_abbreviate_list.append(QString("SPOUS.OF"));
    name_abbreviate_list.append(QString("WIFE.OF"));


    // Compare for aut
    name_abbreviate_for_aut_map[QString("BA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.A")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.PHIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LLB.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.B.A")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LL.B")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LL.B.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.ED.UNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BENG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.ENG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.REL.ED.UNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.SC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BSCMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BSCN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.BIOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.ART")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.RER.NAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.PHIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.PTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.IUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACC.REL.PAED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.RER.SOC.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.\\(FH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.-ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DI")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.-ING.\\(FH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DI\\(FH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.MED.VET")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.UNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.DENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LIC.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.ARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.BIOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.SC.HUM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.ART")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.RER.NAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.PHARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.PHIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.PHIL.FAC.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.PSYCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.PTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.IUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.IUR.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.REL.PAED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.RER.SOC.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.DES.IND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.IUR.RER.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.\\(FH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.PHIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LLM.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSTAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.A")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.A.I.S")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.A.\\(ECON.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.B.A")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MDES")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MHPE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.ED.UNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MIBI")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LLM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LL.M")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MLBT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.SC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSCMF")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSCN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("E.MA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EMLE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EMPH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EMBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LLM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.E.S")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAGASTROSOPHY")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MIM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPOS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MTD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MMH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MMEDSCAA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MBF")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.B.L")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MCF")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MDSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MENG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MEM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MFP")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MFA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MHE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MOHE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MLE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MLS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MLL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MME")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSPHT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MTOX")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MASTERE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PMPH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PMBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PM.ME")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PLL.M")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PMML")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PMM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PMPB")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PMSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PHD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SC.INF.BIOMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.NAT.TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SC.HUM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.ARTIUM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SCIENT.MED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MONT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.NAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.CUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.PHIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.PHIL.FAC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SCIENT.PTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.IUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.SOC.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.VET")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.RER.NAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BSTAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.A.\\(ECON.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.KOMM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.SOZ")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAKK.SPORT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.-DOLM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.-KFM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.-KFFR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.VW")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.-VW")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.DOLM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DKFFR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DKFM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.COMM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.UNIV.ETSCIENT.MED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.UNIV.ETMED.DENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.PHARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.POL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.DENT.ETSCIENT.MED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.KOMM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MR.PHARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.SOZ")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.SPORT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MA.RER.NAT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSCPH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TZT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("APOTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PHARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ARTS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BIO-ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING.AGR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("D.ARTS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("IR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("T.ARTS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("GEAGGR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AGR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SPEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAK.PR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("IKON")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SPEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("D-R")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("D-RNAUK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEKN.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FOLKESK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SOCIALR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ERGOT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FYSIOT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SYGEPL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("JORDEM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.SCIENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PH.D")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DVM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LICENCE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("L")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DPLG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING.DIPL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MASTER")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MST")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MIAGE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DESS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DHE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EEMBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DEA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.HABIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PT.(T.E.)")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.MICH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("METAPT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOTT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.JUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.MED.ETCHIR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.ODONT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.OECON")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.PHARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("C.S.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.THEOL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPAED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOTT.SSA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOTT.MAG.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOTT.SSAMAG.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MU1")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MU2")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOTT.RIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOTT.SSARIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACC.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACC.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.MED.TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.DENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.STOM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.MED.VET")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.ART")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("STOM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.SPEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BSCARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSCARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("HAB.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("STHD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ALLMENNL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACHELOR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CAND.PAED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SIV.ARK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SIV.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LIC.JUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LIC.TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MGR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LEK.DENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LEK.MED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LEK.STOM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LEK.WET")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRHAB")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LICENCIADO")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LICENCIADA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MESTRE")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DOUTOR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("INST")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ECON.COL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING.COL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SUBING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ECON")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ECON.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FARM.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FIL.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("IUR.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MED.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ODONT.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEOL.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AGR.MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("APOT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BERGSING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CIV.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CIV.EKON")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EKON.MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FIL.MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MED.MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEKN.MAG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AGR.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AGRL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EKON.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FARM.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FARML")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FIL.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("IUR.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("JL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MED.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ML")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEKN.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEKNL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEOL.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SKOG.LIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SKOGL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AGR.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AGRD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MED.DR.VET")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MED.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EKON.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FARM.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FARMD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FIL.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("IUR.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("JD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ODONT.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("OD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SKOG.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("SKOGD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEKN.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEKND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BDENTMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BLAW")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BVETMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MCHIROMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MDENTMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MLAW")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.OEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MUDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MVDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MDDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING.ARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MGR.ART")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PHARMDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PHDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PAEDDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("JUDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("RNDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("THDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ARTD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CSC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.EKON.VED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAG.FARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PROF")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("GRDO")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("GRDA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ARQU")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MU")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MIU")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BCA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MGA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("RSDR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TH.D")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("THLIC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.JUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.RER.POL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DLA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.UNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("LIS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DDA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("NP..")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("INXH.DIPL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("JUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MSHK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MJEK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MJEKSTOM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DSTH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("AN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BACH.UNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PER")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CONT.PUBL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIS.IND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING.CIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ESPEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAING.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.VET")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MR.DIPL.ING")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.SCI")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ENG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("CIR.-DENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("METSN.DR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("METSN.KAND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("HD")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MAJ")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("KARSH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("KARSH.A.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.D")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("B.ED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M.ED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MPHIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TECN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MR.PH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DENT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("ING.IND")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MTRO")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MTRA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.DEMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.ABIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BAPP")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRMED")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRSTOM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.EKON")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.INFORM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.PRAV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPL.D-RSTOM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("M-R")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.ART")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("PREP")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TECHN")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("VR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("INFORM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DRUM.")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BTECH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MTECH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DTECH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DUK")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("HEKIM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("Y.LIS")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("VET.HEKIMI")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DR.FIL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("FANL.NOMZ")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DIPLARCH")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MJUR")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("MUNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DBA")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("DUNIV")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BL")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BPHARM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BSP")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BTEC")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BT")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("BVM")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("EC.ENG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("VET.SURG")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("TEST")] = AUT_STR_COMPARE_FLAG;
    name_abbreviate_for_aut_map[QString("IUR.")] = AUT_STR_COMPARE_FLAG;



    //name_abbreviate_list.append(QString("USAGE"));
    name_abbreviate_for_aut_list.append(QString("^(DIPL|DR|B)\\.[\\.\\-A-Z]{1,}$"));
}

int ReplaceSpecialChar(const QString input_str_0, QString special_character_str, QString &output_str)
{
    int pos = 0;
    QVector<QChar> special_character_vector;
    QRegExp special_character(special_character_str);
    while((pos = special_character.indexIn(input_str_0, pos)) != -1)
    {
        int pos_t = special_character.pos(0);
        int len_t = special_character.matchedLength();
        QChar tt = input_str_0[pos_t];
#if DETAIL_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("Posizition: '%1' Char: '%2' unicode:'%3'").arg(pos_t).arg(tt).arg(tt.unicode()); 
#endif
        special_character_vector.append(tt);
        pos += len_t;
    }

    /*
    for (int i=0; i < special_character_vector.count(); i++)
    {
        QChar tt = special_character_vector[i];
        QString replaced_code("?");
        if(special_unicode_transform_table_0.contains(tt.unicode()))
            replaced_code = special_unicode_transform_table_0[tt.unicode()];
        viz_id_number.replace(tt, replaced_code);
        qDebug() << "viz_id_number:: Replace special Code " << tt << " : " << viz_id_number; 
    }
    */
    int special_character_count = special_character_vector.count();
    QString input_str_0_t = input_str_0;
    for (int i = 0; i < special_character_count; i++)
    {
        QChar tt = special_character_vector[i];
        QString replaced_code("([A-Z0-9]*)");
        input_str_0_t.replace(tt, replaced_code);
    }
    output_str = input_str_0_t;
    output_str.replace(QRegExp("^"), "^").replace(QRegExp("$"), "$");

    return special_character_count;
}

// According input_str_1 to change input_str_0, and output matched flag and output_str
//   ABC#CDE  matched ABC98CDE,  # match 98                              output: matched,    output_str = ABC98CDE
//   ABC#CDE  matched ABCCDE,  # match nothing, will be replaced by ?    output: notmatched, output_str = ABC?CDE
//
//
bool ReplaseSpecialChar(const QString input_str_0, const QString input_str_1, int &special_character_count, QString &output_str)
{
    int pos = 0;
    QVector<QChar> special_character_vector;
    QRegExp special_character("[^A-Za-z0-9]");
    while((pos = special_character.indexIn(input_str_0, pos)) != -1)
    {
        int pos_t = special_character.pos(0);
        int len_t = special_character.matchedLength();
        QChar tt = input_str_0[pos_t];
#if DETAIL_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("Posizition: '%1' Char: '%2' unicode:'%3'").arg(pos_t).arg(tt).arg(tt.unicode()); 
#endif
        special_character_vector.append(tt);
        pos += len_t;
    }

    /*
    for (int i=0; i < special_character_vector.count(); i++)
    {
        QChar tt = special_character_vector[i];
        QString replaced_code("?");
        if(special_unicode_transform_table_0.contains(tt.unicode()))
            replaced_code = special_unicode_transform_table_0[tt.unicode()];
        viz_id_number.replace(tt, replaced_code);
        qDebug() << "viz_id_number:: Replace special Code " << tt << " : " << viz_id_number; 
    }
    */
    special_character_count = special_character_vector.count();
    QString input_str_0_t = input_str_0;
    for (int i = 0; i < special_character_count; i++)
    {
        QChar tt = special_character_vector[i];
        QString replaced_code("([A-Z0-9]*)");
        input_str_0_t.replace(tt, replaced_code);
    }
#if DETAIL_INFO_LOG
    qDebug() << __FUNCTION__ << __LINE__ << QString("input_str_0:: pattern: '%1'").arg(input_str_0_t); 
#endif
    QRegExp mat_c(input_str_0_t);

    bool folder_match_flag;
    QMap<unsigned int, QString> matched_string;

    if (mat_c.indexIn(input_str_1) != -1) {
        folder_match_flag = true;
#if DETAIL_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("input_str_1:: '%1' matched '%2'").arg(input_str_1).arg(input_str_0_t);
#endif
        for (int i_t = 1; i_t < special_character_count + 1; i_t++)
        {
            QString matched_str_t = mat_c.cap(i_t);
#if DETAIL_INFO_LOG
            qDebug() <<  __FUNCTION__ << __LINE__ << QString("input_str_0:: special char matched string '%1' in input_str_1").arg(matched_str_t); 
#endif
            if(matched_str_t.length() != 0)
            {
                matched_string[i_t - 1] = matched_str_t;
            }
        }

        {
            QMap<unsigned int, QString>::iterator iter = matched_string.begin();
            while (iter != matched_string.end())
            {
                unsigned int key_t = iter.key();
                QString val_t = iter.value();
#if DETAIL_INFO_LOG
                qDebug() <<  __FUNCTION__ << __LINE__ << QString("matched_string:: '%1' to '%2'").arg(key_t).arg(val_t);
#endif
                iter++;
            }
        }

        QString input_str_0_t_1 = input_str_0;
        for (int i = 0; i < special_character_count; i++)
        {
            QChar tt = special_character_vector[i];
            if(matched_string.contains(i))
            {
                input_str_0_t_1.replace(tt, matched_string[i]);
            }
            else
            {
                QString replaced_code("?");
                input_str_0_t_1.replace(tt, replaced_code);
            }
        }

        output_str = input_str_0_t_1;
#if DETAIL_INFO_LOG
        qDebug() << "first_id_number:: " << input_str_0_t_1; 
#endif
        return true;
    }
    else
    {
        QString input_str_0_t_1 = input_str_0;
        for (int i=0; i < special_character_vector.count(); i++)
        {
            QChar tt = special_character_vector[i];
            QString replaced_code("?");
            input_str_0_t_1.replace(tt, replaced_code);
        }
        output_str = input_str_0_t_1;
        return false;
    }
}


int ParseIDNumber(QString viz_id_number, QString mrz_id_number, QString country_area_str, QString &first_id_number, QString &second_id_number)
{
    int res = 0;
    QString output_id_number;
    if(mrz_id_number.isEmpty())
    {
        first_id_number = viz_id_number;
        second_id_number = mrz_id_number;
        res = 2;
        goto Done;
    }

    if(viz_id_number.isEmpty())
    {
        first_id_number = mrz_id_number;
        second_id_number = viz_id_number;
        res = 2;
        goto Done;
    }

    qDebug() << __FUNCTION__ << __LINE__ << "id:: VID : " << viz_id_number; 
    qDebug() << __FUNCTION__ << __LINE__ << "id:: MRZ : " << mrz_id_number; 

    // toUpper
    viz_id_number = viz_id_number.toUpper();
    mrz_id_number = mrz_id_number.toUpper();
    

    // Remove NO NR from viz_id_number    
    viz_id_number.replace("NO", "").replace("NR", "");
#if DETAIL_ID_INFO_LOG
    qDebug() << "viz_id_number:: Remove NO or NR: " << viz_id_number; 
#endif
    
    int special_character_count;
    bool matched_flag = ReplaseSpecialChar(viz_id_number, mrz_id_number, special_character_count, output_id_number);
    
    if(special_character_count == 0)
    {
#if DETAIL_ID_INFO_LOG
        qDebug() << "viz_id_number:: No Special Char: " << viz_id_number; 
#endif
        int viz_len = viz_id_number.length();
        int mrz_len = mrz_id_number.length();
        if(viz_len == mrz_len)
        {
            if(QString::compare(viz_id_number, mrz_id_number) == 0)
                first_id_number = mrz_id_number;
            else
                first_id_number = viz_id_number;
            res = 1;
            goto Done;
        }
        else
        {
            first_id_number = mrz_id_number;
            second_id_number = viz_id_number;
            res = 2;
            goto Done;
        }
    }
    else
    {
        if(matched_flag)
        {
            first_id_number = output_id_number;
        }
        else
        {
#if DETAIL_ID_INFO_LOG
            qDebug() << "viz_id_number:: not matched : " << viz_id_number << " " << mrz_id_number; 
#endif
            first_id_number = output_id_number;
            res = 2;
            goto Done;
        }
    }
    
Done:
    if(QString::compare(country_area_str, QString("LBN")) == 0)
    {
        first_id_number = QString("RL") + first_id_number;
    }

    return res;
}


int MatchedCharCount(const QString str_0, const QString str_1)
{
    if(str_0.length() != str_1.length())
        return 0;
    int matched_count = 0;
    int length = str_0.length();
    for (int i = 0; i < length; ++i) {
        if(str_0.at(i) == str_1.at(i))
            matched_count++;
    }
    return matched_count;
}

QString CommonSubStr(const QString str_0, const QString str_1)
{
    QString res("");
    int length_0 = str_0.length();
    int length_1 = str_1.length();
    if(length_0 <= length_1)
    {
        //Search substring of str_0 from str_1 
        for(int idx = 0; idx < length_0; idx++)
        {
            QString sub_str = str_0.mid(idx);
            if(str_1.contains(sub_str))
            {
                res = sub_str;
                break;
            }
        }
    }
    else
    {
        //Search substring of str_1 from str_0 
        for(int idx = 0; idx < length_1; idx++)
        {
            QString sub_str = str_1.mid(idx);
            if(str_0.contains(sub_str))
            {
                res = sub_str;
                break;
            }
        }
    }
    return res;
}

int BlurCommonStr(const QString str_0, const QString str_1)
{
    int length_0 = str_0.length();
    int length_1 = str_1.length();
    if(length_0 <= length_1)
    {
        for(int idx = 0; idx < length_0; idx++)
        {
            QString str_0_t = str_0;
            QString match_str = str_0_t.replace(idx, 1, QString("([A-Z0-9])"));
#if DETAIL_NAME_INFO_LOG
            qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_str '%1'").arg(match_str);
#endif
            QRegExp mat_c(match_str);
            if (mat_c.indexIn(str_1) != -1) 
            {
                return length_0;
            }
        }
    }
    else
    {
        for(int idx = 0; idx < length_1; idx++)
        {
            QString str_1_t = str_1;
            QString match_str = str_1_t.replace(idx, 1, QString("([A-Z0-9])"));
#if DETAIL_NAME_INFO_LOG
            qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_str '%1'").arg(match_str);
#endif
            QRegExp mat_c(match_str);
            if (mat_c.indexIn(str_0) != -1) 
            {
                return length_1;
            }
        }
    }
    return 0;
}

// AHMED VS AHED
int BlurCompareStr(const QString str_0, const QString str_1)
{
    int length_0 = str_0.length();
    int length_1 = str_1.length();

    for(int idx = 0; idx < length_0; idx++)
    {
        QString str_0_t = str_0;
        QString match_str = str_0_t.replace(idx, 1, QString("[A-Z0-9]{0,1}"));
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_str '%1'").arg(match_str);
#endif
        QRegExp mat_c(match_str);
        if (mat_c.indexIn(str_1) != -1) 
        {
            return 1;
        }
    }

    for(int idx = 0; idx < length_1; idx++)
    {
        QString str_1_t = str_1;
        QString match_str = str_1_t.replace(idx, 1, QString("[A-Z0-9]{0,1}"));
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_str '%1'").arg(match_str);
#endif
        QRegExp mat_c(match_str);
        if (mat_c.indexIn(str_0) != -1) 
        {
            return 1;
        }
    }

    return 0;
}

// VIZ    MRZ
// HVO    HYO
// MRZ
//E/F I/T I/L M/N M/W V/W V/Y C/G Q/O G/Q P/R
// [EF] [ITL] [MNW] [VY] [CGOQ] [PR]
int SimilarCompareStr(const QString str_mrz, const QString str_viz)
{
    int length_0 = str_viz.length();
    int len_mrz = str_mrz.length();
    QString reg_mrz;

    reg_mrz += QString("^");
    for(int idx = 0; idx < len_mrz; idx++)
    {
        bool similar_letter_flag = false;
        foreach (auto similar_letter, similar_letter_list) 
        {
            if(similar_letter.contains(str_mrz[idx]))
            {
                reg_mrz += similar_letter;
                similar_letter_flag = true;
                break;
            }
        }
        if(similar_letter_flag == false)
            reg_mrz += QString(str_mrz[idx]);
    }
    reg_mrz += QString("$");
#if DETAIL_NAME_INFO_LOG
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("str_viz '%1'").arg(str_viz);
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("str_mrz '%1'").arg(reg_mrz);
#endif
    QRegExp mat_c(reg_mrz);
    if (mat_c.indexIn(str_viz) != -1) 
    {
        return 1;
    }
    return 0;

}
int PreprocessName(QString &viz_name, QString &mrz_name)
{
    QString viz_name_t = viz_name;
    QString mrz_name_t = mrz_name;
    QStringList viz_name_group_list = viz_name_t.split(QRegExp("\\s+"));
    QStringList mrz_name_group_list = mrz_name_t.split(QRegExp("\\s+"));
    QStringList mrz_common_str_list;

    QStringList viz_common_str_list;

    QStringList viz_name_group_pattern_list;
    
    QList<int>  viz_name_group_contain_special_char_flag;
    QList<int>  viz_name_group_exactly_matched_flag;

    int viz_name_group_count = viz_name_group_list.size();
    QStringList mrz_name_group_output_list;
    for (int idx = 0; idx < viz_name_group_count; ++idx){
        viz_name_group_contain_special_char_flag.append(0);
        viz_name_group_exactly_matched_flag.append(0);
    }

    for (int idx = 0; idx < viz_name_group_count; ++idx)
    {
        QString viz_name_group = viz_name_group_list[idx];
        QString viz_name_group_pattern;
        int special_char_count = ReplaceSpecialChar(viz_name_group, QString("[^A-Z0-9\\.\\(\\)]"), viz_name_group_pattern);
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("viz_group: '%1' '%2'").arg(viz_name_group).arg(viz_name_group_pattern); 
#endif
        
        if(special_char_count)
        {
            viz_name_group_contain_special_char_flag[idx] = 1;
            viz_name_group_pattern_list.append(viz_name_group_pattern);
        }
        else
        {
            viz_name_group_pattern_list.append(viz_name_group);
        }
    }

    foreach (auto mrz_name_group, mrz_name_group_list) 
    {
        bool exact_matched_flag = false;
        for (int idx = 0; idx < viz_name_group_count; ++idx)
        {
            if(viz_name_group_contain_special_char_flag[idx] == 0)
            {
                QString viz_name_group = viz_name_group_list[idx];
                viz_name_group.replace(QRegExp("[\\.\\(\\)]"), "");
                if(QString::compare(mrz_name_group, viz_name_group) == 0)
                {
                    exact_matched_flag = true;
                    viz_name_group_exactly_matched_flag[idx] = 1;
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("Begin to match '%1', use pattern '%2' Exactly match").arg(mrz_name_group).arg(viz_name_group); 
#endif
                    break;
                }
            }
            else
            {
                QString viz_name_group = viz_name_group_pattern_list[idx];

                QString match_pattern = QString("%1").arg(viz_name_group);
                QRegExp mat_c(match_pattern);
                if (mat_c.indexIn(mrz_name_group) != -1) 
                {
                    exact_matched_flag = true;
                    viz_name_group_exactly_matched_flag[idx] = 1;
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("Begin to match '%1', use pattern '%2' QRegExp Exactly match").arg(mrz_name_group).arg(viz_name_group); 
#endif
                    break;
                }
            }

        }

        if(exact_matched_flag)
        {
            continue;
        }
            
        bool matched_flag = 0;
        int mrz_name_group_length = mrz_name_group.length();
        for (int idx = 0; idx < viz_name_group_count; ++idx)
        {
            if(viz_name_group_exactly_matched_flag[idx])
                continue;
            if(viz_name_group_contain_special_char_flag[idx] == 0)
            {
                QString viz_name_group = viz_name_group_list[idx];
                int viz_name_group_length = viz_name_group.length();
#if DETAIL_NAME_INFO_LOG
                qDebug() << __FUNCTION__ << __LINE__ <<  QString("Begin to match '%1', use pattern '%2'").arg(mrz_name_group).arg(viz_name_group); 
#endif
                //if(mrz_name_group_length <= 2)
                //    continue;
                QString common_str = CommonSubStr(mrz_name_group, viz_name_group);
                int common_str_length = common_str.length();
                if(common_str_length >= 1 && (abs(mrz_name_group_length - common_str_length) <= 0 || abs(viz_name_group_length - common_str_length) <= 0))
                {
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("Matched common str '%1'").arg(common_str); 
#endif
                    matched_flag = 1;
                    if(common_str_length < mrz_name_group_length)
                    {
                        mrz_common_str_list.append(common_str);
                    }
                    else if(common_str_length < viz_name_group_length)
                    {
                        QString viz_name_group_t = viz_name_group;
                        viz_name_group_t.replace(common_str, "");
                        if(mrz_name_group_list.contains(viz_name_group_t))
                            viz_common_str_list.append(common_str);
                    }
                }
            }
        }
    }

    foreach (auto mrz_common_str, mrz_common_str_list) 
    {
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_common_str: '%1'").arg(mrz_common_str); 
#endif
        mrz_name.replace(mrz_common_str, QString("%1 ").arg(mrz_common_str));
    }

    foreach (auto viz_common_str, viz_common_str_list)
    { 
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("viz_common_str: '%1'").arg(viz_common_str); 
#endif
        viz_name.replace(viz_common_str, QString("%1 ").arg(viz_common_str));
    }

    return 0;
}

QString RemoveSpecialChar(QString input_str)
{
    input_str.replace(".", "");
    return input_str;
}

#define TOKEN_NORMAL_FLAG    0
#define TOKEN_LEFT_PARENTHESES_FLAG    1
#define TOKEN_RIGHT_PARENTHESES_FLAG   2
#define TOKEN_AT_FLAG   3
#define TOKEN_IGNORE_FLAG   8

QString TOKEN_FLAG[] = {
    "TOKEN_NORMAL_FLAG",                  // 0
    "TOKEN_LEFT_PARENTHESES_FLAG",        // 1
    "TOKEN_RIGHT_PARENTHESES_FLAG",       // 2
    "TOKEN_AT_FLAG",                      // 3
    "",
    "",
    "",
    "",
    "TOKEN_IGNORE_FLAG"                   // 8
};

#define MRZ_GROUP_MATCH_FLAG_NO_MATCHED 0
#define MRZ_GROUP_MATCH_FLAG_MATCHED 1
#define MRZ_GROUP_MATCH_FLAG_ALREADY_MATCHED_AND_IGNORE 2


#define VIZ_GROUP_MATCH_FLAG_NO_MATCHED 0
#define VIZ_GROUP_MATCH_FLAG_MATCHED 1
#define VIZ_GROUP_MATCH_FLAG_ALREADY_MATCHED_AND_IGNORE 2

int ParseName(QString viz_name, QString mrz_name, QString &right_name, const QString country_area_str, QString viz_surname)
{    
    int viz_name_group_count;

    if(viz_name.length() == 0 && mrz_name.length() != 0)
    {
        right_name = QString("");
        right_name = mrz_name;
        qDebug() << __FUNCTION__ << __LINE__ << QString("Output-Right-Name: set mrz_name '%1' to right_name because viz_name is null and  mrz_name is not null").arg(mrz_name); 
        return 1;
    }

    if(viz_name.length() == 0 || mrz_name.length() == 0)
    {
        right_name = QString("");
        qDebug() << __FUNCTION__ << __LINE__ << QString("Output-Right-Name: '%1' because viz_name or mrz_name is null").arg(right_name); 
        return 1;
    }

    viz_name = viz_name.trimmed();
    mrz_name = mrz_name.trimmed();
    viz_surname = viz_surname.trimmed();
    

    viz_name = viz_name.toUpper();
    mrz_name = mrz_name.toUpper();
    viz_surname = viz_surname.toUpper();

    if(QString::compare(country_area_str, "AUT") == 0)
    {
        viz_surname.replace(QRegExp("\\s+\\."), ".");
        viz_name.replace(QRegExp("\\s+\\."), ".");
    }

    viz_surname.replace("EP.", "EP. ");
    viz_name.replace("EP.", "EP. ");

    viz_surname.replace("^", " ");

    // Process surname_assist
    QString surname_assist_token_list[] = {
        QString("EP."),
        QString("NEE"),
        QString("VVE."),
        QString("GEB."),
        QString("G/V"),
        QString("E/V"),
        QString("USAGE"),
        QString("")
    };
    QList<int>  surname_assist_token_flag_list;

    int surname_assist_token_idx = 0;
    while(surname_assist_token_list[surname_assist_token_idx].length() != 0)
    {
        QString surname_assist_token = surname_assist_token_list[surname_assist_token_idx];
        QRegExp mat_c_0(QString("^%1\\s+[A-Z]+").arg(surname_assist_token));
        QRegExp mat_c_1(QString("\\s+%1\\s+[A-Z]+").arg(surname_assist_token));
        int flag_0 = 0;
        int flag_1 = 0;
        flag_0 = mat_c_0.indexIn(viz_surname);
        flag_1 = mat_c_1.indexIn(viz_surname);
        if (flag_0 != -1 || flag_1 != -1)
        {
            int usage_idx = viz_name.indexOf(surname_assist_token);
            if(usage_idx >= 0)
            {
                QString viz_name_b = viz_name.left(usage_idx);
                QString viz_name_a = viz_name.mid(usage_idx);
                viz_name_a.replace("-", "");
                viz_name = viz_name_b + viz_name_a;
                qDebug() << __FUNCTION__ << __LINE__ << QString("viz_name Replace - after %1 : ").arg(surname_assist_token) << viz_name; 
            }
            surname_assist_token_flag_list.append(1);
        }
        else
        {
            surname_assist_token_flag_list.append(0);
        }
        surname_assist_token_idx++;
    }

    if(QString::compare(country_area_str, "AUT") == 0)
    {
        //viz_name.replace("-", " ");
        //viz_name.replace(".", " ");
        //viz_name.replace("(", " ( ");
        viz_name.replace(")", " ) ");
    }
    else
    {
        viz_name.replace("-", " ");
        //viz_name.replace(".", " ");
        viz_name.replace("(", " ( ");
        viz_name.replace(")", " ) ");
    }

    viz_name.replace("^", " ");
    viz_name.replace(",", " ");
    viz_name.replace("@", " @ ");
    viz_name.replace("_", " ");

    mrz_name.replace("_", " ");
    
    viz_name = viz_name.trimmed();


    qDebug() << __FUNCTION__ << __LINE__ << "name:: VID : " << viz_name; 
    qDebug() << __FUNCTION__ << __LINE__ << "name:: MRZ : " << mrz_name; 

    qDebug() << __FUNCTION__ << __LINE__ << "name:: VIZ_SURNAME : " << viz_surname; 

    if(viz_name.length() == 0)
    {
        right_name = mrz_name.left(50);
        qDebug() << __FUNCTION__ << __LINE__ << QString("Output-Right-Name: '%1'").arg(right_name); 
        return 1;
    }

    PreprocessName(viz_name, mrz_name);

    viz_name = viz_name.trimmed();
    mrz_name = mrz_name.trimmed();

    qDebug() << __FUNCTION__ << __LINE__ << "After Preprocess VID : " << viz_name; 
    qDebug() << __FUNCTION__ << __LINE__ << "After Preprocess MRZ : " << mrz_name; 

    // sparate token by space 
    //viz_name.replace("^", "");
    //viz_name.replace(".", "");
    //viz_name.replace("USBAND OF", "USBAND.OF").replace("SPOUS OF", "SPOUS.OF").replace("WIFE OF", "WIFE.OF");

    QStringList  viz_name_group_list = viz_name.split(QRegExp("\\s+"));
    QList<int>  viz_name_group_matched_flag;
    QList<int>   viz_name_group_contain_special_char_flag;
    QStringList  viz_name_group_pattern_list;
    QList<int>   viz_name_group_extra_info_flag;

    QStringList  mrz_name_group_list = mrz_name.split(QRegExp("\\s+"));
    QList<int>  mrz_name_group_matched_flag;
    QList<int>   mrz_name_group_matched_idx;

    QStringList mrz_name_group_output_list;

    viz_name_group_count = viz_name_group_list.size();

    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
    {
        viz_name_group_matched_flag.append(VIZ_GROUP_MATCH_FLAG_NO_MATCHED);
        viz_name_group_contain_special_char_flag.append(0);
        if(viz_name_group_list[viz_name_group_idx].contains("("))
        {
            viz_name_group_extra_info_flag.append(TOKEN_LEFT_PARENTHESES_FLAG);
        }
        else if(viz_name_group_list[viz_name_group_idx].contains(")"))
        {
            viz_name_group_extra_info_flag.append(TOKEN_RIGHT_PARENTHESES_FLAG);
        }
        else if(viz_name_group_list[viz_name_group_idx].contains("@"))
        {
            viz_name_group_extra_info_flag.append(TOKEN_AT_FLAG);
        }
        else
        {
            viz_name_group_extra_info_flag.append(TOKEN_NORMAL_FLAG);
        }
    }

    int b_tmp = TOKEN_NORMAL_FLAG;
    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
    {
        if(viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_LEFT_PARENTHESES_FLAG)
        {
            b_tmp = TOKEN_IGNORE_FLAG;
        }
        else if(viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_RIGHT_PARENTHESES_FLAG)
        {
            b_tmp = TOKEN_NORMAL_FLAG;
        }
        else
        {
            if(viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_AT_FLAG)
            {

            }
            else
            {
                viz_name_group_extra_info_flag[viz_name_group_idx] = b_tmp;
            }
        }
    }

    b_tmp = TOKEN_NORMAL_FLAG;
    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx){
        if(viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_AT_FLAG)
        {
            b_tmp = TOKEN_IGNORE_FLAG;
        }
        else
        {
            if(viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                viz_name_group_extra_info_flag[viz_name_group_idx] = b_tmp;
        }
    }

    // Process surname_assist
    int surname_assist_token_flag_list_length = surname_assist_token_flag_list.length();
    for(int surname_assist_token_idx = 0; surname_assist_token_idx < surname_assist_token_flag_list_length; surname_assist_token_idx++)
    {
        int flag = surname_assist_token_flag_list[surname_assist_token_idx];
        if(flag)
        {
            QString surname_assist_token = surname_assist_token_list[surname_assist_token_idx];
            int viz_name_group_idx = 0;
            while(viz_name_group_idx < viz_name_group_count)
            {
                QString viz_name_group = viz_name_group_list[viz_name_group_idx];
                if(QString::compare(viz_name_group, surname_assist_token) == 0)
                {
                    viz_name_group_extra_info_flag[viz_name_group_idx] = TOKEN_IGNORE_FLAG;
                    if(viz_name_group_idx + 1 < viz_name_group_count)
                    {
                        if(viz_name_group_extra_info_flag[viz_name_group_idx + 1] == TOKEN_NORMAL_FLAG)
                            viz_name_group_extra_info_flag[viz_name_group_idx + 1] = TOKEN_IGNORE_FLAG;
                        viz_name_group_idx++;
                    }
                }
                viz_name_group_idx++;
            }
        }

    }

    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
    {
        QString viz_name_group = viz_name_group_list[viz_name_group_idx];
        QString viz_name_group_pattern;
        int special_char_count = ReplaceSpecialChar(viz_name_group, QString("[^A-Z0-9\\.\\(\\)]"), viz_name_group_pattern);
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("viz_name_group: '%1' viz_name_group_pattern:'%2' extra_info_flag:%3")
                      .arg(viz_name_group).arg(viz_name_group_pattern).arg(TOKEN_FLAG[viz_name_group_extra_info_flag[viz_name_group_idx]]); 
#endif
        viz_name_group_pattern_list.append(viz_name_group_pattern);
        if(special_char_count)
            viz_name_group_contain_special_char_flag[viz_name_group_idx] = 1;
    }

#if DETAIL_NAME_INFO_LOG
    foreach (auto mrz_name_group, mrz_name_group_list) 
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_group: '%1'").arg(mrz_name_group); 

    }
    foreach (auto viz_name_group_contain_special_char, viz_name_group_contain_special_char_flag) 
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("viz_name_group_contain_special_char_flag: '%1'").arg(viz_name_group_contain_special_char); 
    }
#endif

#if DETAIL_NAME_INFO_LOG
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("Some Comments:"); 
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=0 : exactly match"); 
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=1 : similar char match"); 
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : comment sub str"); 
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=3 : BlurCommonStr"); 
    qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=4 : BlurCompareStr"); 
#endif

    viz_name_group_count = viz_name_group_pattern_list.size();

    QString viz_name_group;
    foreach (auto mrz_name_group, mrz_name_group_list) 
    {
        int matched_flag = MRZ_GROUP_MATCH_FLAG_NO_MATCHED;
        int matched_idx = -1;
        int used_viz_name_flag = 0;

        QString viz_name_group_matched;
        int matched_type = -1;

#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ <<  QString("Begin_to_match mrz_name_group:'%1'").arg(mrz_name_group); 
#endif
        int mrz_name_group_length = mrz_name_group.length();

        if(matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(!mrz_name_group_matched_idx.contains(viz_name_group_idx) && 
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    QString viz_name_group_pattern = viz_name_group_pattern_list[viz_name_group_idx];
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, use matched_type=0(exactly match) use pattern '%1'").arg(viz_name_group_pattern); 
#endif
                    if(viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                    {
                        QRegExp mat_c(viz_name_group_pattern);
                        if (mat_c.indexIn(mrz_name_group) != -1) 
                        {
                            matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                            matched_idx = viz_name_group_idx;
                            viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                            used_viz_name_flag = 0;
                            viz_name_group_matched = viz_name_group_pattern;
                            matched_type = 0;
                            break;
                        }
                    }
                    else
                    {
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("Ignore due to %1").arg(viz_name_group_extra_info_flag[viz_name_group_idx]); 
#endif
                    }
                }
            }
        }

#if 1
        if(matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(viz_name_group_contain_special_char_flag[viz_name_group_idx] == 0 && (!mrz_name_group_matched_idx.contains(viz_name_group_idx)) &&
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    viz_name_group = viz_name_group_list[viz_name_group_idx];
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, matched_type=10(SimilarCompareStr)use pattern '%1'").arg(viz_name_group); 
#endif
                    int similar_compare_flag = SimilarCompareStr(mrz_name_group, viz_name_group);

                    if(similar_compare_flag)
                    {
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=SimilarCompareStr : matched");
#endif
                        matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                        matched_idx = viz_name_group_idx;
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                        used_viz_name_flag = 0;
                        viz_name_group_matched = viz_name_group;
                        matched_type = 10;
                        break;
                    }
                }
            }
        }
#endif

        // similar char, for example, ABC BBC is similar
        if(0 && matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(viz_name_group_contain_special_char_flag[viz_name_group_idx] == 0 && (!mrz_name_group_matched_idx.contains(viz_name_group_idx)) &&
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    viz_name_group = viz_name_group_list[viz_name_group_idx];
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, use matched_type=1(similar char match) use pattern '%1'").arg(viz_name_group); 
#endif
                    int matched_char_count = MatchedCharCount(mrz_name_group, viz_name_group);
                    if(matched_char_count != 0 && abs(mrz_name_group_length - matched_char_count) <= 1)
                    {
                        matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                        matched_idx = viz_name_group_idx;
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                        used_viz_name_flag = 1;
                        viz_name_group_matched = viz_name_group;
                        matched_type = 1;
                        break;
                    }

                    if(matched_char_count != 0)
                    {
                        if(mrz_name_group_length >= 8 && abs(mrz_name_group_length - matched_char_count) <= (int)(0.5 * mrz_name_group_length))
                        {
                            matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                            matched_idx = viz_name_group_idx;
                            viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                            used_viz_name_flag = 0;
                            viz_name_group_matched = viz_name_group;
                            matched_type = 1;   
                            break;
                        }
                    }
                }
            }
        }

        // comment sub str
        if(matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(viz_name_group_contain_special_char_flag[viz_name_group_idx] == 0 && (!mrz_name_group_matched_idx.contains(viz_name_group_idx)) &&
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    viz_name_group = viz_name_group_list[viz_name_group_idx];
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, matched_type=2(comment sub str) use pattern '%1'").arg(viz_name_group); 
#endif
                    QString common_str = CommonSubStr(mrz_name_group, viz_name_group);
                    int common_str_length = common_str.length();
                    int viz_name_group_length = viz_name_group.length();
                    if(abs(mrz_name_group_length - common_str_length) <= 0 || abs(viz_name_group_length - common_str_length) <= 0)
                    {
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : Matched common str '%1'").arg(common_str); 
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : mrz_name_group_length '%1'").arg(mrz_name_group_length); 
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : viz_name_group_length '%1'").arg(viz_name_group_length); 
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : common_str_length '%1'").arg(common_str_length); 
#endif
                        matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                        matched_idx = viz_name_group_idx;
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                        if(abs(viz_name_group_length - common_str_length) <= 0)
                        {
                            used_viz_name_flag = 1;
#if DETAIL_NAME_INFO_LOG
                            qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : used_viz_name_flag:%1").arg(used_viz_name_flag); 
#endif
                        }
                            
                        if(abs(mrz_name_group_length - common_str_length) <= 0)
                        {
                            used_viz_name_flag = 0;
                            QString mrz_name_group_pattern = QString("^%1.*").arg(mrz_name_group);
                            QRegExp mat_c(mrz_name_group_pattern);
                            if (mat_c.indexIn(viz_name_group) != -1) {
                                used_viz_name_flag = 1;
                            }
#if DETAIL_NAME_INFO_LOG
                            qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : mrz_name_group_pattern:%1").arg(mrz_name_group_pattern); 
                            qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=2 : used_viz_name_flag:'%1'").arg(used_viz_name_flag); 
#endif
                        }
                            
                        viz_name_group_matched = viz_name_group;
                        matched_type = 2;
                        break;
                    }

                }
            }

            if(matched_flag == MRZ_GROUP_MATCH_FLAG_MATCHED)
            {

            }
        }

        // comment sub str
        if(matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(viz_name_group_contain_special_char_flag[viz_name_group_idx] == 0 && (!mrz_name_group_matched_idx.contains(viz_name_group_idx)) &&
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    viz_name_group = viz_name_group_list[viz_name_group_idx];
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, matched_type=3(BlurCommonStr) use pattern '%1'").arg(viz_name_group); 
#endif
                    int blur_common_str_length = BlurCommonStr(mrz_name_group, viz_name_group);
                    int viz_name_group_length = viz_name_group.length();
                    if(blur_common_str_length >=3 && (abs(mrz_name_group_length - blur_common_str_length) <= 0 || abs(viz_name_group_length - blur_common_str_length) <= 0))
                    {
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=BlurCommonStr : Matched common str length '%1'").arg(blur_common_str_length);
#endif
                        matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                        matched_idx = viz_name_group_idx;
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                        if(abs(viz_name_group_length - blur_common_str_length) <= 0)
                            used_viz_name_flag = 1;
                        if(abs(mrz_name_group_length - blur_common_str_length) <= 0)
                            used_viz_name_flag = 0;

                        //used_viz_name_flag = 1;

                        viz_name_group_matched = viz_name_group;
                        matched_type = 3;
                        break;
                    }

                }
            }
        }

        if(matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(viz_name_group_contain_special_char_flag[viz_name_group_idx] == 0 && (!mrz_name_group_matched_idx.contains(viz_name_group_idx)) &&
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    viz_name_group = viz_name_group_list[viz_name_group_idx];
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, matched_type=4(BlurCompareStr)use pattern '%1'").arg(viz_name_group); 
#endif
                    int blur_compare_flag = BlurCompareStr(mrz_name_group, viz_name_group);

                    if(blur_compare_flag)
                    {
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ <<  QString("matched_type=BlurCompareStr : matched");
#endif
                        matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                        matched_idx = viz_name_group_idx;
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;

                        used_viz_name_flag = 1;
                        used_viz_name_flag = 0;

                        viz_name_group_matched = viz_name_group;
                        matched_type = 4;
                        break;
                    }
                }
            }
        }

        // abbreviate, for example  PHI PHIXFG is similar
        if(matched_flag == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(!mrz_name_group_matched_idx.contains(viz_name_group_idx) &&
                    viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
                    viz_name_group = viz_name_group_list[viz_name_group_idx];
                    QString match_pattern = QString("^%1([A-Z0-9]*)").arg(mrz_name_group);
                    QRegExp mat_c(match_pattern);
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ <<  QString("match_subprocess, matched_type=8(abbreviate) : use pattern '%2'").arg(match_pattern); 
#endif
                    if (mat_c.indexIn(viz_name_group) != -1) {
                        matched_flag = MRZ_GROUP_MATCH_FLAG_MATCHED;
                        matched_idx = viz_name_group_idx;
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                        used_viz_name_flag = 1;
                        viz_name_group_matched = viz_name_group;
                        matched_type = 8;
                        break;
                    }
                }
            }
        }

        if(used_viz_name_flag)
        {
            mrz_name_group_output_list.append(RemoveSpecialChar(viz_name_group));
        }
        else
        {
            mrz_name_group_output_list.append(mrz_name_group);
        }

        mrz_name_group_matched_flag.append(matched_flag);
        mrz_name_group_matched_idx.append(matched_idx);
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << 
                QString("End_to_match mrz_group: '%1' matched_type:%2 matched_flag:%3 matched_str:'%4'").arg(mrz_name_group)
                    .arg(matched_type).arg(matched_flag)
                    .arg(viz_name_group_matched); 
#endif
    }

    // remove in name_abbreviate_list
    // remove AUT in name_abbreviate_for_aut
    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
    {
        if(viz_name_group_matched_flag[viz_name_group_idx] == VIZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
            viz_name_group = viz_name_group_list[viz_name_group_idx];
            if(name_abbreviate_list.contains(viz_name_group))
            {
#if DETAIL_NAME_INFO_LOG
                qDebug() << __FUNCTION__ << __LINE__ << QString("name_abbreviate_list contain '%1'").arg(viz_name_group);
#endif
                viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_ALREADY_MATCHED_AND_IGNORE;
            }
            
            if(QString::compare(country_area_str, "AUT") == 0)
            {
#if DETAIL_NAME_INFO_LOG
                qDebug() << __FUNCTION__ << __LINE__ << QString("Check AUT list contain '%1'").arg(viz_name_group);
#endif
                if(name_abbreviate_for_aut_map.contains(viz_name_group))
                {
                    viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_ALREADY_MATCHED_AND_IGNORE;
                    continue;
                }

                foreach (auto name_abbreviate_for_aut, name_abbreviate_for_aut_list) 
                {
                    QRegExp mat_c(name_abbreviate_for_aut);
                    if (mat_c.indexIn(viz_name_group) != -1) {
                        viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_ALREADY_MATCHED_AND_IGNORE;
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ << QString("AUT: %1 matched %2").arg(viz_name_group).arg(name_abbreviate_for_aut);
#endif
                        break;
                    }
                    else
                    {
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ << QString("AUT: %1 not matched %2").arg(viz_name_group).arg(name_abbreviate_for_aut);
#endif
                    }
                }
            }
        }
#if DETAIL_NAME_INFO_LOG
        qDebug() << __FUNCTION__ << __LINE__ << QString("viz_group_matched: '%1'").arg(viz_name_group_matched_flag[viz_name_group_idx]); 
#endif
    }

    int mrz_name_group_count = mrz_name_group_matched_flag.size();
    for (int mrz_name_group_idx = 0; mrz_name_group_idx < mrz_name_group_count; ++mrz_name_group_idx)
    {
        if(mrz_name_group_matched_flag[mrz_name_group_idx] != MRZ_GROUP_MATCH_FLAG_MATCHED)
        {
            for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
            {
                if(viz_name_group_matched_flag[viz_name_group_idx] == VIZ_GROUP_MATCH_FLAG_NO_MATCHED && 
                   viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
                {
#if DETAIL_NAME_INFO_LOG
                    qDebug() << __FUNCTION__ << __LINE__ << QString("Replace not matched mrz_name '%1' with viz_name_group '%2'")
                        .arg(mrz_name_group_output_list[mrz_name_group_idx]).arg(viz_name_group_list[viz_name_group_idx]);
#endif
                    mrz_name_group_output_list[mrz_name_group_idx] = RemoveSpecialChar(viz_name_group_list[viz_name_group_idx]);
                    mrz_name_group_matched_flag[mrz_name_group_idx] = MRZ_GROUP_MATCH_FLAG_MATCHED;
                    mrz_name_group_matched_idx[mrz_name_group_idx] = viz_name_group_idx;
                    viz_name_group_matched_flag[viz_name_group_idx] = VIZ_GROUP_MATCH_FLAG_MATCHED;
                    break;
                }
            }
        }
    }

#if DETAIL_NAME_INFO_LOG
    for (int mrz_name_group_idx = 0; mrz_name_group_idx < mrz_name_group_count; ++mrz_name_group_idx)
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_name_group:%1 mrz_name_group_matched_flag:%2")
            .arg(mrz_name_group_list[mrz_name_group_idx]).arg(mrz_name_group_matched_flag[mrz_name_group_idx]);
    }
#endif

    // Check if mrz_group is matched
    for (int mrz_name_group_idx = 0; mrz_name_group_idx < mrz_name_group_count; ++mrz_name_group_idx)
    {
        if(mrz_name_group_matched_flag[mrz_name_group_idx] == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
        {
#if DETAIL_NAME_INFO_LOG
            qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_name_group_matched_flag:%1 mrz_name_group_matched_idx:%2 mrz_name_group_output_list:'%3'")
                .arg(mrz_name_group_matched_flag[mrz_name_group_idx]).arg(mrz_name_group_matched_idx[mrz_name_group_idx]).arg(mrz_name_group_output_list[mrz_name_group_idx]);
#endif
            for (int idx_t = 0; idx_t < mrz_name_group_count; ++idx_t)
            {
#if DETAIL_NAME_INFO_LOG
                qDebug() << __FUNCTION__ << __LINE__ << QString("Check_if_matched: '%1' match mrz_name_group_matched_flag:%2 mrz_name_group_matched_idx:%3 mrz_name_group_output_list:'%4'")
                    .arg(mrz_name_group_output_list[mrz_name_group_idx]).arg(mrz_name_group_matched_flag[idx_t]).arg(mrz_name_group_matched_idx[idx_t]).arg(mrz_name_group_output_list[idx_t]);
#endif
                if(mrz_name_group_matched_flag[idx_t])
                {
                    if(QString::compare(mrz_name_group_output_list[mrz_name_group_idx], mrz_name_group_list[idx_t]) == 0)
                    {
                        mrz_name_group_matched_flag[mrz_name_group_idx] = MRZ_GROUP_MATCH_FLAG_ALREADY_MATCHED_AND_IGNORE;
#if DETAIL_NAME_INFO_LOG
                        qDebug() << __FUNCTION__ << __LINE__ << QString("Check_if_matched: '%1' is checked, ignore it")
                            .arg(mrz_name_group_output_list[mrz_name_group_idx]);
#endif                  
                        break;
                    }
                }
            }

#if DETAIL_NAME_INFO_LOG
            qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_name_group_matched_flag:%1 mrz_name_group_matched_idx:%2 mrz_name_group_output_list:'%3'")
                .arg(mrz_name_group_matched_flag[mrz_name_group_idx]).arg(mrz_name_group_matched_idx[mrz_name_group_idx]).arg(mrz_name_group_output_list[mrz_name_group_idx]);
#endif

        }
    }

#if DETAIL_NAME_INFO_LOG
    for (int mrz_name_group_idx = 0; mrz_name_group_idx < mrz_name_group_count; ++mrz_name_group_idx)
    {
        if(mrz_name_group_matched_flag[mrz_name_group_idx])
        {
            int viz_matched_idx = mrz_name_group_matched_idx[mrz_name_group_idx];
            if(viz_matched_idx >= 0)
                qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_name_group_matched_flag:%1 mrz_name_group_matched_idx:%2 mrz_name_group_output_list:'%3' viz_name_group:'%4'")
                    .arg(mrz_name_group_matched_flag[mrz_name_group_idx]).arg(mrz_name_group_matched_idx[mrz_name_group_idx]).arg(mrz_name_group_output_list[mrz_name_group_idx]).arg(viz_name_group_list[viz_matched_idx]);
            else
                qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_name_group_matched_flag:%1 mrz_name_group_matched_idx:%2 mrz_name_group_output_list:'%3'")
                    .arg(mrz_name_group_matched_flag[mrz_name_group_idx]).arg(mrz_name_group_matched_idx[mrz_name_group_idx]).arg(mrz_name_group_output_list[mrz_name_group_idx]);
        }
        else
        {
            qDebug() << __FUNCTION__ << __LINE__ << QString("mrz_name_group_matched_flag:%1 mrz_name_group_matched_idx:%2 mrz_name_group_output_list:'%3'")
                .arg(mrz_name_group_matched_flag[mrz_name_group_idx]).arg(mrz_name_group_matched_idx[mrz_name_group_idx]).arg(mrz_name_group_output_list[mrz_name_group_idx]);

        }
    }


    viz_name_group_count = viz_name_group_matched_flag.size();
    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("idx:%1 viz_name_group_matched_flag:%2 viz_name_group:'%3'")
            .arg(viz_name_group_idx).arg(viz_name_group_matched_flag[viz_name_group_idx]).arg(viz_name_group_list[viz_name_group_idx]);

    }
#endif

    QStringList mrz_name_group_output_list_tmp;
    for (int mrz_name_group_idx = 0; mrz_name_group_idx < mrz_name_group_count; ++mrz_name_group_idx)
    {
        if(mrz_name_group_matched_flag[mrz_name_group_idx] == MRZ_GROUP_MATCH_FLAG_MATCHED)
        {
            mrz_name_group_output_list_tmp.append(mrz_name_group_output_list[mrz_name_group_idx]);
        }
    }

    viz_name_group_count = viz_name_group_matched_flag.size();
    for (int viz_name_group_idx = 0; viz_name_group_idx < viz_name_group_count; ++viz_name_group_idx)
    {
        if(viz_name_group_matched_flag[viz_name_group_idx] == VIZ_GROUP_MATCH_FLAG_NO_MATCHED && viz_name_group_extra_info_flag[viz_name_group_idx] == TOKEN_NORMAL_FLAG)
        {
            mrz_name_group_output_list_tmp.append(RemoveSpecialChar(viz_name_group_list[viz_name_group_idx]));
        }
    }

    // add mrz group which is not matched and not in viz group
    bool apend_redundant_mrz_group_flag;
    
    if(arab_country_area_list.contains(country_area_str))
    {
        apend_redundant_mrz_group_flag = false;
    }
    else
    { 
        apend_redundant_mrz_group_flag = true;
    }

    //apend_redundant_mrz_group_flag = false;

    if(apend_redundant_mrz_group_flag)
    {
        for (int mrz_name_group_idx = 0; mrz_name_group_idx < mrz_name_group_count; ++mrz_name_group_idx)
        {
            if(mrz_name_group_matched_flag[mrz_name_group_idx] == MRZ_GROUP_MATCH_FLAG_NO_MATCHED)
            {
                mrz_name_group_output_list_tmp.append(mrz_name_group_output_list[mrz_name_group_idx]);
            }
        }
    }


    right_name = mrz_name_group_output_list_tmp.join(" ");
    right_name = right_name.left(50);
    qDebug() << __FUNCTION__ << __LINE__ << QString("Output-Right-Name: '%1'").arg(right_name); 
    return 1;
}

int ParseExactMatchString(QString str_viz, QString str_mrz, QString &out_str)
{
    if(str_viz.isEmpty())
    {
        out_str = str_mrz;
        return 2;
    }

    if(str_mrz.isEmpty())
    {
        out_str = str_viz;
        return 2;
    }
    
    if(QString::compare(str_viz, str_mrz) == 0)
    {
        out_str = str_viz;
    }
    else
    {
        out_str = str_mrz;
    }
    return 2; 
}

int ParseCountryArea(QString viz_countryarea, QString mrz_countryarea, QString &right_countryarea)
{
    ParseExactMatchString(viz_countryarea, mrz_countryarea, right_countryarea);

    if(QString::compare(right_countryarea, "RKS") == 0)
    {
        right_countryarea = QString("XXX");
    }
    return 0;
}

int ParseGender(QString viz_gender, QString mrz_gender, QString &right_gender)
{
    ParseExactMatchString(viz_gender, mrz_gender, right_gender);
    if(QString::compare(right_gender, QString("F")) == 0)
        right_gender = "2";
    else if(QString::compare(right_gender, QString("M")) == 0)
        right_gender = "1";
    else
        right_gender = "9";
    return 0;
}

// Check issue rules
bool ParseQueryInfo(const QString str, QString &description)
{
    bool res = true;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError || doc.isNull()) {
        qDebug() << __FUNCTION__ << __LINE__ << "Json format is wrong." << jsonError.error;
        res = false;
        return res;
    }
    QJsonObject rootObj = doc.object();

    QJsonValue ParamValue = rootObj.value("description");
    if (ParamValue.type() == QJsonValue::String) 
    {
        res = true;
        description = ParamValue.toString();
    }
    else
    {
        res = false;
        description = QString("");
    }
    return res;
}

// Check issue rules
bool ParseQueryInfo(const QString str, QMap<QString, QString> &key_map, QList<QMap<QString, QString> > &rules_map_list)
{
    bool res = true;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError || doc.isNull()) {
        //qDebug() << __FUNCTION__ << __LINE__ << "Json format is wrong." << jsonError.error;
        res = false;
        return res;
    }
    QJsonObject rootObj = doc.object();

    QJsonValue ParamValue = rootObj.value("RulesParam");
    if (ParamValue.type() == QJsonValue::Object) {
        QJsonObject ParamObj = ParamValue.toObject();
        QJsonValue JSonValue_t;

        QJsonObject::Iterator it;

        for(it = ParamObj.begin(); it != ParamObj.end(); it++)
        {
            QString key_obj = it.key();
            QJsonValue val_obj = it.value();
            if(val_obj.isString())
            {
                key_map[key_obj] = val_obj.toString();
            }
            else if(val_obj.isArray())
            {
                QStringList val_list;
                QJsonArray json_array_val = val_obj.toArray();
                int nSize = json_array_val.size();
                for (int i = 0; i < nSize; ++i) 
                {
                    QJsonValue value_t0 = json_array_val.at(i);
                    if (value_t0.type() == QJsonValue::Object)
                    {
                        QMap<QString, QString> rule_map;
                        QJsonObject ParamObj_t000 = value_t0.toObject();
                        QJsonObject::Iterator it_tt;

                        for(it_tt = ParamObj_t000.begin(); it_tt != ParamObj_t000.end(); it_tt++)
                        {
                            QString key_t0_obj = it_tt.key();
                            QJsonValue val_t0_obj = it_tt.value();
                            if(val_t0_obj.isString())
                            {
                                rule_map[key_t0_obj] = val_t0_obj.toString();
                            }
                        }
                        rules_map_list.append(rule_map);
                    }
                }
            }
        }
    }
    else
    {
        qDebug() << __FUNCTION__ << __LINE__ << "Json format is wrong no RulesParam json node";
        res = false;
    }

    return res;
}


// Check issue rules
QString GetQueryResInfo(QMap<QString, QString> &key_map, QList<QMap<QString, QString> > &rules_map_list)
{
    QJsonDocument jdoc;
    QJsonObject obj;

    QJsonObject obj_t;
    QMap<QString, QString>::iterator iter = key_map.begin();
    while (iter != key_map.end())
    {
        QJsonObject Member;
        obj_t[iter.key()] = iter.value();
        iter++;
    }

    QJsonArray obj_a_0;
    foreach (auto rules_map, rules_map_list) {
        QJsonObject obj_t_0;
        QMap<QString, QString>::iterator iter_t = rules_map.begin();
        while (iter_t != rules_map.end())
        {
            QJsonObject Member;
            obj_t_0[iter_t.key()] = iter_t.value();
            iter_t++;
        }

        obj_a_0.append(obj_t_0);
    }

    obj_t["data"] = obj_a_0;
    obj["RulesResParam"] = obj_t;

    jdoc.setObject(obj);


    QString content(jdoc.toJson(QJsonDocument::Indented));
    return content;
}


static bool CheckIssueRule_type_0(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    bool check_flag = true;
    precondition_satisfied_flag = false;
    //1-M-1900:1999-14
    // Like this:
    /*
    Precondition:    1- first number of Preson Number
    Checkcondition:  M  Male
    Checkcondition:  1900:1999 Birthday data
    */
    QStringList rule_str_list = rule_str.split("-");
    if(rule_str_list.length() >= 4)
    {
        QString  Person_number_str = rule_str_list[0];
        QString  Gender_str = rule_str_list[1];
        QString  Birthday_str = rule_str_list[2];

        if(query_rules_info_map.contains("ID") && query_rules_info_map.contains("Birthday") && query_rules_info_map.contains("Gender"))
        {
            QMap<QString, QVariant> mrz_content_map;
            mrz_content_map = query_rules_info_map["mrz_all_info_json"].toMap();

            if(mrz_content_map.contains("xml_post_token_for_person_number"))
            {
                const QString Person_number_org_str = mrz_content_map["xml_post_token_for_person_number"].toString();
                const QString ID_org_str = query_rules_info_map["ID"].toString();
                const QString Gender_org_str = query_rules_info_map["Gender"].toString();
                const QString Birthday_org_str = query_rules_info_map["Birthday"].toString();

                qDebug() << __FUNCTION__ << __LINE__ << QString("Check PersonNumber:%1; Gender:%2; Birthday:%3;").arg(Person_number_org_str).arg(Gender_org_str).arg(Birthday_org_str);
                qDebug() << __FUNCTION__ << __LINE__ << QString("Match PersonNumber:%1; Gender:%2; Birthday:%3;").arg(Person_number_str).arg(Gender_str).arg(Birthday_str);

                QString match_pattern = QString("^%1.*").arg(Person_number_str);
                QRegExp mat_c(match_pattern);
                if (mat_c.indexIn(Person_number_org_str) != -1) 
                {
                    // Satisfy pre condition
                    // Precondition ios satisfied.
                    precondition_satisfied_flag = true;
                    if(QString::compare(Gender_str, "M") == 0)
                    {
                        if(QString::compare(Gender_org_str, "1") != 0)
                        {
                            check_flag = false;
                            goto Done;
                        }
                    }
                    else if(QString::compare(Gender_str, "F") == 0)
                    {
                        if(QString::compare(Gender_org_str, "2") != 0)
                        {
                            check_flag = false;
                            goto Done;
                        }
                    }

                    if(Birthday_org_str.length() == 8)
                    {
                        QString year_str = Birthday_org_str.mid(0, 4);
                        int year_org = year_str.toInt();

                        QString min_year_str = Birthday_str.mid(0, 4);
                        QString max_year_str = Birthday_str.mid(5, 4);
                        int min_year = min_year_str.toInt();
                        int max_year = max_year_str.toInt();
                        if(year_org >= min_year && year_org <= max_year)
                        {

                        }
                        else
                        {
                            check_flag = false;
                            goto Done;
                        }
                    }
                    //ID_type
                }
            }
        }
    }
Done:    
    return check_flag;
}

static bool CheckIssueRule_type_1(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    bool check_flag = true;
    precondition_satisfied_flag = false;
    // PE-1:3-ABC-1M
    // PE, ID 1 到 3 位, 为 ABC，有效期 1M 月，YMD
    // Like this:
    /*
    Precondition:    PE- type
    Checkcondition:  1:3-ABC
    Checkcondition:  1M
    */
    QStringList rule_str_list = rule_str.split("-");
    if(rule_str_list.length() >= 4)
    {
        QString  id_type_str = rule_str_list[0];
        QString  id_range_str = rule_str_list[1];
        QString  id_range_content_str = rule_str_list[2];
        QString  expire_date_str = rule_str_list[3];

        if(query_rules_info_map.contains("ID") && query_rules_info_map.contains("ID_type"))
        {
            QMap<QString, QVariant> mrz_content_map;
            mrz_content_map = query_rules_info_map["mrz_all_info_json"].toMap();

            QMap<QString, QVariant> viz_content_map;
            viz_content_map = query_rules_info_map["viz_all_info_json"].toMap();
            
            if(viz_content_map.contains("xml_post_token_for_date_of_expiry") && viz_content_map.contains("xml_post_token_for_date_of_issue"))
            {
                const QString ID_org_str = query_rules_info_map["ID"].toString();
                const QString ID_type_org_str = query_rules_info_map["ID_type"].toString();

                qDebug() << __FUNCTION__ << __LINE__ << QString("Check ID:%1; ID_Type:%2;").arg(ID_org_str).arg(ID_type_org_str);

                if(QString::compare(ID_type_org_str, id_type_str) == 0)
                {
                    // Satisfy pre condition
                    // Precondition ios satisfied.
                    precondition_satisfied_flag = true;

                    QStringList range_str_list = id_range_str.split(":");
                    if(range_str_list.length() >= 2)
                    {
                        int range_min = range_str_list[0].toInt();
                        int range_max = range_str_list[1].toInt();
                        if(range_min <= range_max)
                        {
                            int range_len = range_max - range_min + 1;
                            QString id_ort_part_str = ID_org_str.mid(range_min - 1, range_len);
                            if(QString::compare(id_range_content_str, id_ort_part_str) != 0)
                            {
                                qDebug() << __FUNCTION__ << __LINE__ << QString("Check failed %1 != %2;").arg(id_range_content_str).arg(id_ort_part_str);
                                check_flag = false;
                                goto Done;
                            }
                        }
                    }

                    const QString date_of_expiry = viz_content_map["xml_post_token_for_date_of_expiry"].toString();
                    const QString date_of_issue = viz_content_map["xml_post_token_for_date_of_issue"].toString();

                    int32_t expiry_year_str = date_of_expiry.mid(0, 4).toInt();
                    int32_t expiry_month_str = date_of_expiry.mid(5, 2).toInt();

                    int32_t issue_year_str = date_of_issue.mid(0, 4).toInt();
                    int32_t issue_month_str = date_of_issue.mid(5, 2).toInt();

                    int32_t month_count = expiry_year_str * 12 + expiry_month_str - issue_year_str * 12 - issue_month_str;

                    qDebug() << __FUNCTION__ << __LINE__ << QString("Check %1-%2 %3-%4;").arg(expiry_year_str).arg(expiry_month_str).arg(issue_year_str).arg(issue_month_str);

                    int32_t expire_date_str_len = expire_date_str.length();

                    if(QString::compare(expire_date_str.mid(expire_date_str_len - 1, 1), "M") == 0)
                    {
                        int32_t m_t = expire_date_str.mid(0, expire_date_str_len - 1).toInt();
                        if(m_t != month_count)
                        {
                            qDebug() << __FUNCTION__ << __LINE__ << QString("Check failed %1 != %2;").arg(m_t).arg(month_count);
                            check_flag = false;
                            goto Done;
                        }
                    }
                    if(QString::compare(expire_date_str.mid(expire_date_str_len - 1, 1), "Y") == 0)
                    {
                        int32_t m_t = expire_date_str.mid(0, expire_date_str_len - 1).toInt();
                        m_t = m_t * 12;
                        if(m_t != month_count)
                        {
                            qDebug() << __FUNCTION__ << __LINE__ << QString("Check failed %1 != %2;").arg(m_t).arg(month_count);
                            check_flag = false;
                            goto Done;
                        }
                    }
                }
            }
        }

    }
Done:
    return check_flag;
}

static bool CheckIssueRule_type_2(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    bool check_flag = true;
    precondition_satisfied_flag = false;
    // PD-D
    // Like this:
    QStringList rule_str_list = rule_str.split("-");
    if(rule_str_list.length() >= 2)
    {
        QString  id_type_str = rule_str_list[0];
        QString  id_first_letter_str = rule_str_list[1];
        
        if(query_rules_info_map.contains("ID") && query_rules_info_map.contains("ID_type"))
        {
            const QString ID_org_str = query_rules_info_map["ID"].toString();
            const QString ID_type_org_str = query_rules_info_map["ID_type"].toString();

            qDebug() << __FUNCTION__ << __LINE__ << QString("Check ID:%1; ID_Type:%2;").arg(ID_org_str).arg(ID_type_org_str);
            if(QString::compare(ID_type_org_str, id_type_str) == 0)
            {
                // Satisfy pre condition
                // Precondition ios satisfied.
                precondition_satisfied_flag = true;

                QString match_pattern = QString("^%1.*").arg(id_first_letter_str);
                QRegExp mat_c(match_pattern);
                if (mat_c.indexIn(ID_org_str) != -1) 
                {
                }
                else
                {
                    check_flag = false;
                    goto Done;
                }
            }
        }
    }
Done:    
    return check_flag;
}

static bool CheckIssueRule_type_3(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    bool check_flag = true;
    precondition_satisfied_flag = true;
    // 2-36:37-V1
    // Like this:
    QStringList rule_str_list = rule_str.split("-");
    if(rule_str_list.length() >= 3)
    {
        QString  line_idx_str = rule_str_list[0];
        QString  range_str = rule_str_list[1];
        QString  range_content_str = rule_str_list[2];

        QMap<QString, QVariant> mrz_content_map;
        mrz_content_map = query_rules_info_map["mrz_all_info_json"].toMap();
        if(mrz_content_map.contains("xml_post_token_for_mrz_string"))
        {
            QString mrz_str = mrz_content_map["xml_post_token_for_mrz_string"].toString();
            qDebug() << __FUNCTION__ << __LINE__ << mrz_str;
            QStringList mrz_line_str_list = mrz_str.split(" ");
            if(mrz_line_str_list.length() >= 2)
            {
                QString mrz_str_check;
                if(QString::compare(line_idx_str, "1") == 0)
                {
                    mrz_str_check = mrz_line_str_list[0];
                }
                else if(QString::compare(line_idx_str, "2") == 0)
                {
                    mrz_str_check = mrz_line_str_list[1];
                }
                if(mrz_str_check.length() > 0)
                {
                    QStringList range_str_list = range_str.split(":");
                    int range_min = range_str_list[0].toInt();
                    int range_max = range_str_list[1].toInt();
                    if(range_min <= range_max)
                    {
                        int range_len = range_max - range_min + 1;
                        QString mrz_str_check_part = mrz_str_check.mid(range_min - 1, range_len);
                        qDebug() << __FUNCTION__ << __LINE__ << mrz_str_check_part;
                        if(QString::compare(range_content_str, mrz_str_check_part) != 0)
                        {
                            check_flag = false;
                            goto Done;
                        }
                    }
                }

            }

        }
    }
Done:
    return check_flag;
}

static bool CheckIssueRule_type_4(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    bool check_flag = true;
    precondition_satisfied_flag = false;
    // Like this:
    //PD-D-2020:2023
    QStringList rule_str_list = rule_str.split("-");
    if(rule_str_list.length() >= 3)
    {
        QString  id_type_str = rule_str_list[0];
        QString  id_first_letter_str = rule_str_list[1];
        QString  year_range_str = rule_str_list[2];
        
        if(query_rules_info_map.contains("ID") && query_rules_info_map.contains("ID_type"))
        {
            const QString ID_org_str = query_rules_info_map["ID"].toString();
            const QString ID_type_org_str = query_rules_info_map["ID_type"].toString();

            qDebug() << __FUNCTION__ << __LINE__ << QString("Check ID:%1; ID_Type:%2;").arg(ID_org_str).arg(ID_type_org_str);
            qDebug() << __FUNCTION__ << __LINE__ << QString("Check ID_Type(input):%1;").arg(id_type_str);
            if(QString::compare(ID_type_org_str, id_type_str) == 0)
            {
                // Satisfy pre condition
                // Precondition ios satisfied.
                precondition_satisfied_flag = true;
                qDebug() << __FUNCTION__ << __LINE__ << QString("Check ID first letter:%1;").arg(id_first_letter_str);
                QString match_pattern = QString("^%1.*").arg(id_first_letter_str);
                QRegExp mat_c(match_pattern);
                if (mat_c.indexIn(ID_org_str) != -1) 
                {
                }
                else
                {
                    check_flag = false;
                    goto Done;
                }

                //QMap<QString, QVariant> mrz_content_map;
                //mrz_content_map = query_rules_info_map["mrz_all_info_json"].toMap();
                QMap<QString, QVariant> viz_content_map;
                viz_content_map = query_rules_info_map["viz_all_info_json"].toMap();

                if(viz_content_map.contains("xml_post_token_for_date_of_issue"))
                {
                    const QString date_of_issue = viz_content_map["xml_post_token_for_date_of_issue"].toString();
                    int32_t issue_year = date_of_issue.mid(0, 4).toInt();

                    QStringList year_range_str_list = year_range_str.split(":");

                    if(year_range_str_list.length() >= 2)
                    {
                        int range_min = year_range_str_list[0].toInt();
                        int range_max = year_range_str_list[1].toInt();
                        qDebug() << __FUNCTION__ << __LINE__ << QString("Check IssureYear:%1; Min:%2; Max:%3;").arg(issue_year).arg(range_min).arg(range_max);
                        if(issue_year < range_min || issue_year > range_max)
                        {
                            check_flag = false;
                            goto Done;
                        }
                    }
                }

            }
        }
    }
Done:
    return check_flag;
}

static bool CheckIssueRule_type_5(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    bool check_flag = true;
    precondition_satisfied_flag = false;
    // Like this:
    //PT-4:5-2
    QStringList rule_str_list = rule_str.split("-");
    if(rule_str_list.length() >= 3)
    {
        QString  id_type_str = rule_str_list[0];
        QString  range_str = rule_str_list[1];
        QString  id_first_letter_str = rule_str_list[1];
        
        if(query_rules_info_map.contains("ID") && query_rules_info_map.contains("ID_type"))
        {
            const QString ID_org_str = query_rules_info_map["ID"].toString();
            const QString ID_type_org_str = query_rules_info_map["ID_type"].toString();

            qDebug() << __FUNCTION__ << __LINE__ << QString("Check ID:%1; ID_Type:%2;").arg(ID_org_str).arg(ID_type_org_str);
            if(QString::compare(ID_type_org_str, id_type_str) == 0)
            {
                // Satisfy pre condition
                // Precondition ios satisfied.
                precondition_satisfied_flag = true;

                //QMap<QString, QVariant> mrz_content_map;
                //mrz_content_map = query_rules_info_map["mrz_all_info_json"].toMap();

                QMap<QString, QVariant> viz_content_map;
                viz_content_map = query_rules_info_map["viz_all_info_json"].toMap();

                if(viz_content_map.contains("xml_post_token_for_date_of_issue"))
                {
                    const QString date_of_issue = viz_content_map["xml_post_token_for_date_of_issue"].toString();
                    QString issue_year_str = date_of_issue.mid(2, 2);

                    QStringList range_str_list = range_str.split(":");
                    if(range_str_list.length() >= 2)
                    {
                        int range_min = range_str_list[0].toInt();
                        int range_max = range_str_list[1].toInt();
                        if(range_min < range_max)
                        {
                            QString check_str = ID_org_str.mid(range_min - 1, range_max - range_min + 1);
                            if(QString::compare(check_str, issue_year_str) != 0)
                            {
                                qDebug() << __FUNCTION__ << __LINE__ << QString("Check failed %1 != %2;").arg(check_str).arg(issue_year_str);
                                check_flag = false;
                                goto Done;
                            }
                        }
                    }

                }

            }
        }
    }
Done:
    return check_flag;
}

bool CheckIssueRule_checkDate()
{
    bool res = false;
    QString endDate = QDateTime(QDate(2023, 12, 30), QTime(10, 39, 0)).toString("yyyy-MM-dd");;
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    if (endDate < currentDate)
    {
        res = true;
    }
    return res;
}

typedef bool(*CheckIssueRule_Fun)(QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag);

bool CheckIssueRules(int rule_type, QString rule_str, const QMap<QString, QVariant> query_rules_info_map, bool &precondition_satisfied_flag)
{
    precondition_satisfied_flag = false;
    CheckIssueRule_Fun check_fun[] = {
        CheckIssueRule_type_0,
        CheckIssueRule_type_1,
        CheckIssueRule_type_2,
        CheckIssueRule_type_3,
        CheckIssueRule_type_4,
        CheckIssueRule_type_5

    };
    int rule_type_max = (sizeof(check_fun) / sizeof(CheckIssueRule_Fun));
    if(rule_type < rule_type_max)
    {
        int rule_type_t = rule_type % rule_type_max;
        if(check_fun[rule_type_t] != NULL)
        {
            return check_fun[rule_type_t](rule_str, query_rules_info_map, precondition_satisfied_flag);
        }
    }
    return true;
}

/*
QString base64_encode(QString string){
    QByteArray ba;
    ba.append(string);
    return ba.toBase64();
}

QString base64_decode(QString string){
    QByteArray ba;
    ba.append(string);
    return QByteArray::fromBase64(ba);
}
*/

const QString ReadFileContent(const QString file_name)
{
    QFile file(file_name);
    QString file_content_str;
    bool isok = file.open(QIODevice::ReadOnly);
    if (isok)
    {
        QByteArray array = file.readAll();
        file_content_str = QString(array);
    }
    file.close();
    return file_content_str;
}

bool WriteToFle(const QString file_name, const QString content)
{
    QFile file(file_name);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(content.toUtf8());
    file.close();
    return true;
}

QString fileMd5(const QString &sourceFilePath) 
{
    QFile sourceFile(sourceFilePath);
    qint64 fileSize = sourceFile.size();
    const qint64 bufferSize = 10240;

    if (sourceFile.open(QIODevice::ReadOnly)) {
        char buffer[bufferSize];
        int bytesRead;
        int readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash(QCryptographicHash::Md5);

        while (readSize > 0 && (bytesRead = sourceFile.read(buffer, readSize)) > 0) {
            fileSize -= bytesRead;
            hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);
        }

        sourceFile.close();
        return QString(hash.result().toHex());
    }
    return QString();
}


int processCount(const char*  processName)
{
    int countProcess = 0;
    HANDLE toolHelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (((int)toolHelp32Snapshot) != -1)
    {
        PROCESSENTRY32 processEntry32;
        processEntry32.dwSize = sizeof(processEntry32);
        if(Process32First(toolHelp32Snapshot, &processEntry32))
        {
            do
            {
                if (strcmp(processName, processEntry32.szExeFile) == 0)
                    countProcess++;
            }while (Process32Next(toolHelp32Snapshot, &processEntry32));
        }
        CloseHandle(toolHelp32Snapshot);
    }
    
    return countProcess;
}

bool isProcessAlive(QString targetExePath)
{
    QString exeName = targetExePath.split('/').last();
    QDateTime strtTime = QDateTime::currentDateTime();
    int countProcess = processCount(exeName.toStdString().c_str());  
    if (countProcess == 0)
        return false;
    else
        return true;
}

void ExecuteProcess(QString targetExePath) 
{
    QString exeName = targetExePath.split('/').last();
    QDateTime strtTime = QDateTime::currentDateTime();
    int countProcess = processCount(exeName.toStdString().c_str());  
    if (countProcess == 0)
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("%1 is not alive, so launch it").arg(targetExePath);
        WinExec(targetExePath.toStdString().c_str(), SW_SHOW);
    }
}

int ftp_get(void *network_access_manager, 
            const QString ftp_host,
            int ftp_port,
            const QString ftp_username,
            const QString ftp_pw,
            int timeout, 
            QString ftp_path, 
            QString dst_file)
{

    QEventLoop eventLoop;

    QNetworkAccessManager *manager = new QNetworkAccessManager();

    //QNetworkAccessManager *manager = (QNetworkAccessManager *)network_access_manager;
    QUrl url;
    url.setScheme("ftp");
    url.setHost(ftp_host);             // 设置服务器IP地址
    url.setPort(ftp_port);             // 设置端口
    url.setUserName(ftp_username);     // 设置用户名
    url.setPassword(ftp_pw);           // 设置密码

    url.setPath(ftp_path); 

    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request); 

    QTimer timer;    
    timer.setSingleShot(true);


    int res = 0;
    QObject::connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &eventLoop, SLOT(quit()));
    //QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &eventLoop, SLOT(error(QNetworkReply::NetworkError)));

    timer.start(timeout);   // sec
    
    eventLoop.exec(); 

    if(timer.isActive()) {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(dst_file); 
            file.open(QIODevice::WriteOnly);
            file.write(reply->readAll()); 
            file.close();
            res = 0;
        }
        else 
        {
            qWarning() << __FUNCTION__ << __LINE__ << QString("download failed"); 
            res = -1;
        }
    } else {
        // timeout
        QObject::disconnect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        reply->abort();
        qWarning() << __FUNCTION__ << __LINE__ << QString("download timeout"); 
        res = -1;
    }
    delete reply;
    delete manager;

    return res;
}

int ftp_upload(void *network_access_manager, 
                const QString ftp_host,
                int ftp_port,
                const QString ftp_username,
                const QString ftp_pw,
                int timeout, 
                QString ftp_path, 
                QString src_file)
{
    int res = 0;
    QFile *data = new QFile(src_file);
    if (data->open(QIODevice::ReadOnly)) 
    {
        QEventLoop eventLoop;

        QNetworkAccessManager *manager = new QNetworkAccessManager();

        //QNetworkAccessManager *manager = (QNetworkAccessManager *)network_access_manager;
        QUrl url;
        url.setScheme("ftp");
        url.setHost(ftp_host);             // 设置服务器IP地址
        url.setPort(ftp_port);             // 设置端口
        url.setUserName(ftp_username);     // 设置用户名
        url.setPassword(ftp_pw);           // 设置密码

        url.setPath(ftp_path); 
    
        QNetworkRequest request(url);
        QNetworkReply* reply = manager->put(request, data);

        QTimer timer;
        timer.setSingleShot(true);

        
        QObject::connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
        QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &eventLoop, SLOT(quit()));
        //QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &eventLoop, SLOT(error(QNetworkReply::NetworkError)));

        timer.start(timeout);   // sec
        
        eventLoop.exec(); 

        if(timer.isActive()) {
            timer.stop();
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << __FUNCTION__ << __LINE__ << QString("upload OK"); 
                res = 0;
            }
            else 
            {
                qWarning() << __FUNCTION__ << __LINE__ << QString("upload failed"); 
                res = -1;
            }
        } else {
            // timeout
            QObject::disconnect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
            reply->abort();
            qWarning() << __FUNCTION__ << __LINE__ << QString("upload timeout"); 
            res = -1;
        }
        delete reply;
        delete manager;
        data->close();
    }
    else
    {
        res = -1;
    }
    delete data;

    return res;
}


QStringList unzip_file(const char *FILEPATH, int &err_flag)
{
    // zip文件路径    
    QStringList unzip_file_list;
    unzFile zFile;
    zFile = unzOpen64(FILEPATH);
    //zFile = unzOpen("v1.0.9.Release.zip");
    if (zFile == NULL)
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("open %1 failed").arg(FILEPATH); 
        err_flag = -1;
        return unzip_file_list;
    }

    unz_global_info64 zGlobalInfo;
    if (UNZ_OK != unzGetGlobalInfo64(zFile, &zGlobalInfo))
    {
        qDebug() << __FUNCTION__ << __LINE__ << QString("get global info failed"); 
        err_flag = -1;
        return unzip_file_list;
    }

#define MAX_PATH_LEN 2048
    //unz_file_info* pFileInfo = new unz_file_info;

    unz_file_info pFileInfo;
    char szZipFName[MAX_PATH_LEN];
    int nReturnValue;
    for(int i = 0; i < zGlobalInfo.number_entry; i++)
    {
        nReturnValue = unzGetCurrentFileInfo(zFile, &pFileInfo, szZipFName, MAX_PATH_LEN, NULL, 0, NULL, 0);
        if(nReturnValue != UNZ_OK)
        {
            err_flag = -1;
            return unzip_file_list;
        }

        //判断是文件夹还是文件
        switch(pFileInfo.external_fa)
        {
        case FILE_ATTRIBUTE_DIRECTORY:
            {
                QString strDiskFile = QString("./update.tmp/%1").arg(szZipFName);
                std::string t_z = qstr2str(strDiskFile);
                CreateDirectory(t_z.c_str(), NULL);
            }
            break;
        default:                                        //文件
            {
                //创建文件
                QString strDiskFile = QString("./update.tmp/%1").arg(szZipFName);
                std::string t_z = qstr2str(strDiskFile);
                HANDLE hFile = CreateFile(t_z.c_str(), GENERIC_WRITE,
                    0, NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
                if(hFile == INVALID_HANDLE_VALUE)
                {
                    qDebug() << __FUNCTION__ << __LINE__ << QString("create %1  failed").arg(szZipFName);
                    goto ERR;
                }

                //打开文件
                nReturnValue = unzOpenCurrentFile(zFile);
                if(nReturnValue != UNZ_OK)
                {
                    qDebug() << __FUNCTION__ << __LINE__ << QString("open %1 in zip failed").arg(szZipFName);
                    CloseHandle(hFile);
                    goto ERR;
                }

                //读取文件
                const int BUFFER_SIZE = 4096;
                char szReadBuffer[BUFFER_SIZE];
                while(TRUE)
                {
                    memset(szReadBuffer, 0, BUFFER_SIZE);
                    int nReadFileSize = unzReadCurrentFile(zFile, szReadBuffer, BUFFER_SIZE);
                    if(nReadFileSize < 0)                //读取文件失败
                    {
                        qDebug() << __FUNCTION__ << __LINE__ << QString("read data from %1 in zip failed").arg(szZipFName);
                        unzCloseCurrentFile(zFile);
                        CloseHandle(hFile);
                        goto ERR;
                    }
                    else if(nReadFileSize == 0)            //读取文件完毕
                    {
                        unzCloseCurrentFile(zFile);
                        CloseHandle(hFile);
                        unzip_file_list.append(strDiskFile);
                        break;
                    }
                    else                                //写入读取的内容
                    {
                        DWORD dWrite = 0;
                        BOOL bWriteSuccessed = WriteFile(hFile, szReadBuffer, nReadFileSize, &dWrite, NULL);
                        if(!bWriteSuccessed)
                        {
                            qDebug() << __FUNCTION__ << __LINE__ << QString("write data from %1 in zip failed").arg(szZipFName);
                            unzCloseCurrentFile(zFile);
                            CloseHandle(hFile);
                            goto ERR;
                        }
                    }
                }
            }
            break;
        }
        unzGoToNextFile(zFile);
    }

#undef MAX_PATH_LEN

    //关闭
    if(zFile)
    {
        unzClose(zFile);
    }
    err_flag = 0;
    return unzip_file_list;
ERR:
    if(zFile)
    {
        unzClose(zFile);
    }
    err_flag = -1;
    return unzip_file_list;
}

COREDLL_NAME_SPACE_END