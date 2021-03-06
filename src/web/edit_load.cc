/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    edit_load.cc - this file is part of MediaTomb.
    
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

/// \file edit_load.cc

#include "pages.h"

#include "cds_objects.h"
#include "common.h"
#include "storage/storage.h"
#include "util/tools.h"
#include <cstdio>

//#include "server.h"
//#include "content_manager.h"

using namespace zmm;
using namespace mxml;

web::edit_load::edit_load(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
}

void web::edit_load::process()
{
    check_request();

    std::string objID = param("object_id");
    int objectID;
    if (objID.empty())
        throw std::runtime_error("invalid object id");
    else
        objectID = std::stoi(objID);

    auto obj = storage->loadObject(objectID);

    Ref<Element> item(new Element("item"));

    item->setAttribute("object_id", objID, mxml_int_type);

    Ref<Element> title(new Element("title"));
    title->setTextKey("value");
    title->setText(obj->getTitle());
    title->setAttribute("editable", obj->isVirtual() || objectID == CDS_ID_FS_ROOT ? "1" : "0", mxml_bool_type);
    item->appendElementChild(title);

    Ref<Element> classEl(new Element("class"));
    classEl->setTextKey("value");
    classEl->setText(obj->getClass());
    classEl->setAttribute("editable", "1", mxml_bool_type);
    item->appendElementChild(classEl);

    int objectType = obj->getObjectType();
    item->appendTextChild("obj_type", CdsObject::mapObjectType(objectType));

    if (IS_CDS_ITEM(objectType)) {
        auto objItem = std::static_pointer_cast<CdsItem>(obj);

        Ref<Element> description(new Element("description"));
        description->setTextKey("value");
        description->setText(objItem->getMetadata("dc:description"));
        description->setAttribute("editable", "1", mxml_bool_type);
        item->appendElementChild(description);

        Ref<Element> location(new Element("location"));
        location->setTextKey("value");
        location->setText(objItem->getLocation());
        if (IS_CDS_PURE_ITEM(objectType) || !objItem->isVirtual())
            location->setAttribute("editable", "0", mxml_bool_type);
        else
            location->setAttribute("editable", "1", mxml_bool_type);
        item->appendElementChild(location);

        Ref<Element> mimeType(new Element("mime-type"));
        mimeType->setTextKey("value");
        mimeType->setText(objItem->getMimeType());
        mimeType->setAttribute("editable", "1", mxml_bool_type);
        item->appendElementChild(mimeType);

        if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
            Ref<Element> protocol(new Element("protocol"));
            protocol->setTextKey("value");
            protocol->setText(getProtocol(objItem->getResource(0)->getAttribute("protocolInfo")));
            protocol->setAttribute("editable", "1", mxml_bool_type);
            item->appendElementChild(protocol);
        } else if (IS_CDS_ACTIVE_ITEM(objectType)) {
            auto objActiveItem = std::static_pointer_cast<CdsActiveItem>(objItem);

            Ref<Element> action(new Element("action"));
            action->setTextKey("value");
            action->setText(objActiveItem->getAction());
            action->setAttribute("editable", "1", mxml_bool_type);
            item->appendElementChild(action);

            Ref<Element> state(new Element("state"));
            state->setTextKey("value");
            state->setText(objActiveItem->getState());
            state->setAttribute("editable", "1", mxml_bool_type);
            item->appendElementChild(state);
        }
    }

    root->appendElementChild(item);
    //log_debug("serving XML: {}",  root->print().c_str());
}
