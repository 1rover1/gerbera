/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    web_request_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file web_request_handler.cc

#include "web_request_handler.h"
#include "config/config_manager.h"
#include "content_manager.h"
#include "iohandler/mem_io_handler.h"
#include "util/tools.h"
#include "web/pages.h"
#include <ctime>
#include <util/headers.h>

using namespace zmm;
using namespace mxml;

namespace web {

WebRequestHandler::WebRequestHandler(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content,
    std::shared_ptr<SessionManager> sessionManager)
    : RequestHandler(config, storage)
    , content(content)
    , sessionManager(sessionManager)
    , checkRequestCalled(false)
{
}

int WebRequestHandler::intParam(std::string name, int invalid)
{
    std::string value = param(name);
    if (!string_ok(value))
        return invalid;
    else
        return std::stoi(value);
}

bool WebRequestHandler::boolParam(std::string name)
{
    std::string value = param(name);
    return string_ok(value) && (value == "1" || value == "true");
}

void WebRequestHandler::check_request(bool checkLogin)
{
    // we have a minimum set of parameters that are "must have"

    // check if the session parameter was supplied and if we have
    // a session with that id

    checkRequestCalled = true;

    std::string sid = param("sid");
    if (sid.empty())
        throw SessionException("no session id given");

    if ((session = sessionManager->getSession(sid)) == nullptr)
        throw SessionException("invalid session id");

    if (checkLogin && !session->isLoggedIn())
        throw LoginException("not logged in");
    session->access();
}

std::string WebRequestHandler::renderXMLHeader()
{
    return std::string("<?xml version=\"1.0\" encoding=\"") + DEFAULT_INTERNAL_CHARSET + "\"?>\n";
}

void WebRequestHandler::getInfo(const char *filename, UpnpFileInfo *info)
{
    this->filename = filename;

    std::string path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    dict_decode(parameters, &params);

    UpnpFileInfo_set_FileLength(info, -1); // length is unknown

    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_IsReadable(info, 1);

    std::string contentType;

    std::string mimetype;
    std::string returnType = param("return_type");

    if (string_ok(returnType) && returnType == "xml")
        mimetype = MIMETYPE_XML;
    else
        mimetype = MIMETYPE_JSON;

    contentType = mimetype + "; charset=" + DEFAULT_INTERNAL_CHARSET;

    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(contentType.c_str()));
    Headers headers;
    headers.addHeader(std::string{"Cache-Control"}, std::string{"no-cache, must-revalidate"});
    headers.writeHeaders(info);
}

std::unique_ptr<IOHandler> WebRequestHandler::open(enum UpnpOpenFileMode mode)
{
    root = Ref<Element>(new Element("root"));

    std::string error = "";
    int error_code = 0;

    std::string output;
    // processing page, creating output
    try {
        if (!config->getBoolOption(CFG_SERVER_UI_ENABLED)) {
            log_warning("The UI is disabled in the configuration file. See README.");
            error = "The UI is disabled in the configuration file. See README.";
            error_code = 900;
        } else {
            process();

            if (checkRequestCalled) {
                // add current task
                appendTask(root, content->getCurrentTask());

                handleUpdateIDs();
            }
        }
    } catch (const LoginException& e) {
        error = e.what();
        error_code = 300;
    } catch (const ObjectNotFoundException& e) {
        error = e.what();
        error_code = 200;
    } catch (const SessionException& e) {
        error = e.what();
        error_code = 400;
    } catch (const StorageException& e) {
        error = e.getUserMessage();
        error_code = 500;
    } catch (const std::runtime_error& e) {
        error = std::string{"Error: "} + e.what();
        error_code = 800;
    }

    if (!string_ok(error)) {
        root->setAttribute("success", "1", mxml_bool_type);
    } else {
        root->setAttribute("success", "0", mxml_bool_type);
        Ref<Element> errorEl(new Element("error"));
        errorEl->setTextKey("text");
        errorEl->setText(error);

        if (error_code == 0)
            error_code = 899;
        errorEl->setAttribute("code", std::to_string(error_code));
        root->appendElementChild(errorEl);

        log_warning("Web Error: {} {}", error_code, error);
    }

    std::string returnType = param("return_type");
    if (string_ok(returnType) && returnType == "xml") {
#ifdef TOMBDEBUG
        try {
            // make sure we can generate JSON w/o exceptions
            XML2JSON::getJSON(root);
            //log_debug("JSON-----------------------{}", XML2JSON::getJSON(root).c_str());
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
        }
#endif
        output = renderXMLHeader() + root->print();
    } else {
        try {
            output = XML2JSON::getJSON(root);
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
        }
    }

    /*
    try
    {
        printf("%s\n", output.c_str());
        std::string json = XML2JSON::getJSON(root);
        printf("%s\n",json.c_str());
    }
    catch (const std::runtime_error& e)
    {
        e.printStackTrace();
    }
    */

    //root = nullptr;

    auto io_handler = std::make_unique<MemIOHandler>(output);
    io_handler->open(mode);
    return io_handler;
}

std::unique_ptr<IOHandler> WebRequestHandler::open(const char* filename,
    enum UpnpOpenFileMode mode,
    std::string range)
{
    this->filename = filename;
    this->mode = mode;

    std::string path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    dict_decode(parameters, &params);

    return open(mode);
}

void WebRequestHandler::handleUpdateIDs()
{
    // session will be filled by check_request

    std::string updates = param("updates");
    if (string_ok(updates)) {
        Ref<Element> updateIDs(new Element("update_ids"));
        root->appendElementChild(updateIDs);
        if (updates == "check") {
            updateIDs->setAttribute("pending", session->hasUIUpdateIDs() ? "1" : "0", mxml_bool_type);
        } else if (updates == "get") {
            addUpdateIDs(updateIDs, session);
        }
    }
}

void WebRequestHandler::addUpdateIDs(Ref<Element> updateIDsEl, std::shared_ptr<Session> session)
{
    std::string updateIDs = session->getUIUpdateIDs();
    if (string_ok(updateIDs)) {
        log_debug("UI: sending update ids: {}", updateIDs.c_str());
        updateIDsEl->setTextKey("ids");
        updateIDsEl->setText(updateIDs);
        updateIDsEl->setAttribute("updates", "1", mxml_bool_type);
    }
}

void WebRequestHandler::appendTask(Ref<Element> el, std::shared_ptr<GenericTask> task)
{
    if (task == nullptr || el == nullptr)
        return;
    Ref<Element> taskEl(new Element("task"));
    taskEl->setAttribute("id", std::to_string(task->getID()), mxml_int_type);
    taskEl->setAttribute("cancellable", task->isCancellable() ? "1" : "0", mxml_bool_type);
    taskEl->setTextKey("text");
    taskEl->setText(task->getDescription());
    el->appendElementChild(taskEl);
}

std::string WebRequestHandler::mapAutoscanType(int type)
{
    if (type == 1)
        return "ui";
    else if (type == 2)
        return "persistent";
    else
        return "none";
}

} // namespace
