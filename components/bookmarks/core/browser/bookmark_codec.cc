// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/core/browser/bookmark_codec.h"

#include <algorithm>

#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/bookmarks/core/browser/bookmark_model.h"
#include "grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using base::Time;

const char* BookmarkCodec::kRootsKey = "roots";
const char* BookmarkCodec::kRootFolderNameKey = "bookmark_bar";
const char* BookmarkCodec::kOtherBookmarkFolderNameKey = "other";
// The value is left as 'synced' for historical reasons.
const char* BookmarkCodec::kMobileBookmarkFolderNameKey = "synced";
const char* BookmarkCodec::kVersionKey = "version";
const char* BookmarkCodec::kChecksumKey = "checksum";
const char* BookmarkCodec::kIdKey = "id";
const char* BookmarkCodec::kTypeKey = "type";
const char* BookmarkCodec::kNameKey = "name";
const char* BookmarkCodec::kDateAddedKey = "date_added";
const char* BookmarkCodec::kURLKey = "url";
const char* BookmarkCodec::kDateModifiedKey = "date_modified";
const char* BookmarkCodec::kChildrenKey = "children";
const char* BookmarkCodec::kMetaInfo = "meta_info";
const char* BookmarkCodec::kSyncTransactionVersion = "sync_transaction_version";
const char* BookmarkCodec::kTypeURL = "url";
const char* BookmarkCodec::kTypeFolder = "folder";

// Current version of the file.
static const int kCurrentVersion = 1;

BookmarkCodec::BookmarkCodec()
    : ids_reassigned_(false),
      ids_valid_(true),
      maximum_id_(0),
      model_sync_transaction_version_(
          BookmarkNode::kInvalidSyncTransactionVersion) {
}

BookmarkCodec::~BookmarkCodec() {}

base::Value* BookmarkCodec::Encode(BookmarkModel* model) {
  return Encode(model->bookmark_bar_node(),
                model->other_node(),
                model->mobile_node(),
                model->root_node()->GetMetaInfoMap(),
                model->root_node()->sync_transaction_version());
}

base::Value* BookmarkCodec::Encode(
    const BookmarkNode* bookmark_bar_node,
    const BookmarkNode* other_folder_node,
    const BookmarkNode* mobile_folder_node,
    const BookmarkNode::MetaInfoMap* model_meta_info_map,
    int64 sync_transaction_version) {
  ids_reassigned_ = false;
  InitializeChecksum();
  base::DictionaryValue* roots = new base::DictionaryValue();
  roots->Set(kRootFolderNameKey, EncodeNode(bookmark_bar_node));
  roots->Set(kOtherBookmarkFolderNameKey, EncodeNode(other_folder_node));
  roots->Set(kMobileBookmarkFolderNameKey, EncodeNode(mobile_folder_node));
  if (model_meta_info_map)
    roots->Set(kMetaInfo, EncodeMetaInfo(*model_meta_info_map));
  if (sync_transaction_version !=
      BookmarkNode::kInvalidSyncTransactionVersion) {
    roots->SetString(kSyncTransactionVersion,
                     base::Int64ToString(sync_transaction_version));
  }
  base::DictionaryValue* main = new base::DictionaryValue();
  main->SetInteger(kVersionKey, kCurrentVersion);
  FinalizeChecksum();
  // We are going to store the computed checksum. So set stored checksum to be
  // the same as computed checksum.
  stored_checksum_ = computed_checksum_;
  main->Set(kChecksumKey, new base::StringValue(computed_checksum_));
  main->Set(kRootsKey, roots);
  return main;
}

bool BookmarkCodec::Decode(BookmarkNode* bb_node,
                           BookmarkNode* other_folder_node,
                           BookmarkNode* mobile_folder_node,
                           int64* max_id,
                           const base::Value& value) {
  ids_.clear();
  ids_reassigned_ = false;
  ids_valid_ = true;
  maximum_id_ = 0;
  stored_checksum_.clear();
  InitializeChecksum();
  bool success = DecodeHelper(bb_node, other_folder_node, mobile_folder_node,
                              value);
  FinalizeChecksum();
  // If either the checksums differ or some IDs were missing/not unique,
  // reassign IDs.
  if (!ids_valid_ || computed_checksum() != stored_checksum())
    ReassignIDs(bb_node, other_folder_node, mobile_folder_node);
  *max_id = maximum_id_ + 1;
  return success;
}

base::Value* BookmarkCodec::EncodeNode(const BookmarkNode* node) {
  base::DictionaryValue* value = new base::DictionaryValue();
  std::string id = base::Int64ToString(node->id());
  value->SetString(kIdKey, id);
  const base::string16& title = node->GetTitle();
  value->SetString(kNameKey, title);
  value->SetString(kDateAddedKey,
                   base::Int64ToString(node->date_added().ToInternalValue()));
  if (node->is_url()) {
    value->SetString(kTypeKey, kTypeURL);
    std::string url = node->url().possibly_invalid_spec();
    value->SetString(kURLKey, url);
    UpdateChecksumWithUrlNode(id, title, url);
  } else {
    value->SetString(kTypeKey, kTypeFolder);
    value->SetString(kDateModifiedKey,
                     base::Int64ToString(node->date_folder_modified().
                                   ToInternalValue()));
    UpdateChecksumWithFolderNode(id, title);

    base::ListValue* child_values = new base::ListValue();
    value->Set(kChildrenKey, child_values);
    for (int i = 0; i < node->child_count(); ++i)
      child_values->Append(EncodeNode(node->GetChild(i)));
  }
  const BookmarkNode::MetaInfoMap* meta_info_map = node->GetMetaInfoMap();
  if (meta_info_map)
    value->Set(kMetaInfo, EncodeMetaInfo(*meta_info_map));
  if (node->sync_transaction_version() !=
      BookmarkNode::kInvalidSyncTransactionVersion) {
    value->SetString(kSyncTransactionVersion,
                     base::Int64ToString(node->sync_transaction_version()));
  }
  return value;
}

base::Value* BookmarkCodec::EncodeMetaInfo(
    const BookmarkNode::MetaInfoMap& meta_info_map) {
  base::DictionaryValue* meta_info = new base::DictionaryValue;
  for (BookmarkNode::MetaInfoMap::const_iterator it = meta_info_map.begin();
      it != meta_info_map.end(); ++it) {
    meta_info->SetStringWithoutPathExpansion(it->first, it->second);
  }
  return meta_info;
}

bool BookmarkCodec::DecodeHelper(BookmarkNode* bb_node,
                                 BookmarkNode* other_folder_node,
                                 BookmarkNode* mobile_folder_node,
                                 const base::Value& value) {
  if (value.GetType() != base::Value::TYPE_DICTIONARY)
    return false;  // Unexpected type.

  const base::DictionaryValue& d_value =
      static_cast<const base::DictionaryValue&>(value);

  int version;
  if (!d_value.GetInteger(kVersionKey, &version) || version != kCurrentVersion)
    return false;  // Unknown version.

  const base::Value* checksum_value;
  if (d_value.Get(kChecksumKey, &checksum_value)) {
    if (checksum_value->GetType() != base::Value::TYPE_STRING)
      return false;
    if (!checksum_value->GetAsString(&stored_checksum_))
      return false;
  }

  const base::Value* roots;
  if (!d_value.Get(kRootsKey, &roots))
    return false;  // No roots.

  if (roots->GetType() != base::Value::TYPE_DICTIONARY)
    return false;  // Invalid type for roots.

  const base::DictionaryValue* roots_d_value =
      static_cast<const base::DictionaryValue*>(roots);
  const base::Value* root_folder_value;
  const base::Value* other_folder_value = NULL;
  if (!roots_d_value->Get(kRootFolderNameKey, &root_folder_value) ||
      root_folder_value->GetType() != base::Value::TYPE_DICTIONARY ||
      !roots_d_value->Get(kOtherBookmarkFolderNameKey, &other_folder_value) ||
      other_folder_value->GetType() != base::Value::TYPE_DICTIONARY) {
    return false;  // Invalid type for root folder and/or other
                   // folder.
  }
  DecodeNode(*static_cast<const base::DictionaryValue*>(root_folder_value),
             NULL, bb_node);
  DecodeNode(*static_cast<const base::DictionaryValue*>(other_folder_value),
             NULL, other_folder_node);

  // Fail silently if we can't deserialize mobile bookmarks. We can't require
  // them to exist in order to be backwards-compatible with older versions of
  // chrome.
  const base::Value* mobile_folder_value;
  if (roots_d_value->Get(kMobileBookmarkFolderNameKey, &mobile_folder_value) &&
      mobile_folder_value->GetType() == base::Value::TYPE_DICTIONARY) {
    DecodeNode(*static_cast<const base::DictionaryValue*>(mobile_folder_value),
               NULL, mobile_folder_node);
  } else {
    // If we didn't find the mobile folder, we're almost guaranteed to have a
    // duplicate id when we add the mobile folder. Consequently, if we don't
    // intend to reassign ids in the future (ids_valid_ is still true), then at
    // least reassign the mobile bookmarks to avoid it colliding with anything
    // else.
    if (ids_valid_)
      ReassignIDsHelper(mobile_folder_node);
  }

  if (!DecodeMetaInfo(*roots_d_value, &model_meta_info_map_,
                      &model_sync_transaction_version_))
    return false;

  std::string sync_transaction_version_str;
  if (roots_d_value->GetString(kSyncTransactionVersion,
                               &sync_transaction_version_str) &&
      !base::StringToInt64(sync_transaction_version_str,
                           &model_sync_transaction_version_))
    return false;

  // Need to reset the type as decoding resets the type to FOLDER. Similarly
  // we need to reset the title as the title is persisted and restored from
  // the file.
  bb_node->set_type(BookmarkNode::BOOKMARK_BAR);
  other_folder_node->set_type(BookmarkNode::OTHER_NODE);
  mobile_folder_node->set_type(BookmarkNode::MOBILE);
  bb_node->SetTitle(l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_FOLDER_NAME));
  other_folder_node->SetTitle(
      l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_OTHER_FOLDER_NAME));
  mobile_folder_node->SetTitle(
        l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_MOBILE_FOLDER_NAME));

  return true;
}

bool BookmarkCodec::DecodeChildren(const base::ListValue& child_value_list,
                                   BookmarkNode* parent) {
  for (size_t i = 0; i < child_value_list.GetSize(); ++i) {
    const base::Value* child_value;
    if (!child_value_list.Get(i, &child_value))
      return false;

    if (child_value->GetType() != base::Value::TYPE_DICTIONARY)
      return false;

    DecodeNode(*static_cast<const base::DictionaryValue*>(child_value),
               parent, NULL);
  }
  return true;
}

bool BookmarkCodec::DecodeNode(const base::DictionaryValue& value,
                               BookmarkNode* parent,
                               BookmarkNode* node) {
  // If no |node| is specified, we'll create one and add it to the |parent|.
  // Therefore, in that case, |parent| must be non-NULL.
  if (!node && !parent) {
    NOTREACHED();
    return false;
  }

  std::string id_string;
  int64 id = 0;
  if (ids_valid_) {
    if (!value.GetString(kIdKey, &id_string) ||
        !base::StringToInt64(id_string, &id) ||
        ids_.count(id) != 0) {
      ids_valid_ = false;
    } else {
      ids_.insert(id);
    }
  }

  maximum_id_ = std::max(maximum_id_, id);

  base::string16 title;
  value.GetString(kNameKey, &title);

  std::string date_added_string;
  if (!value.GetString(kDateAddedKey, &date_added_string))
    date_added_string = base::Int64ToString(Time::Now().ToInternalValue());
  int64 internal_time;
  base::StringToInt64(date_added_string, &internal_time);

  std::string type_string;
  if (!value.GetString(kTypeKey, &type_string))
    return false;

  if (type_string != kTypeURL && type_string != kTypeFolder)
    return false;  // Unknown type.

  if (type_string == kTypeURL) {
    std::string url_string;
    if (!value.GetString(kURLKey, &url_string))
      return false;

    GURL url = GURL(url_string);
    if (!node && url.is_valid())
      node = new BookmarkNode(id, url);
    else
      return false;  // Node invalid.

    if (parent)
      parent->Add(node, parent->child_count());
    node->set_type(BookmarkNode::URL);
    UpdateChecksumWithUrlNode(id_string, title, url_string);
  } else {
    std::string last_modified_date;
    if (!value.GetString(kDateModifiedKey, &last_modified_date))
      last_modified_date = base::Int64ToString(Time::Now().ToInternalValue());

    const base::Value* child_values;
    if (!value.Get(kChildrenKey, &child_values))
      return false;

    if (child_values->GetType() != base::Value::TYPE_LIST)
      return false;

    if (!node) {
      node = new BookmarkNode(id, GURL());
    } else {
      // If a new node is not created, explicitly assign ID to the existing one.
      node->set_id(id);
    }

    node->set_type(BookmarkNode::FOLDER);
    int64 internal_time;
    base::StringToInt64(last_modified_date, &internal_time);
    node->set_date_folder_modified(Time::FromInternalValue(internal_time));

    if (parent)
      parent->Add(node, parent->child_count());

    UpdateChecksumWithFolderNode(id_string, title);

    if (!DecodeChildren(*static_cast<const base::ListValue*>(child_values),
                        node)) {
      return false;
    }
  }

  node->SetTitle(title);
  node->set_date_added(base::Time::FromInternalValue(internal_time));

  int64 sync_transaction_version = node->sync_transaction_version();
  BookmarkNode::MetaInfoMap meta_info_map;
  if (!DecodeMetaInfo(value, &meta_info_map, &sync_transaction_version))
    return false;
  node->SetMetaInfoMap(meta_info_map);

  std::string sync_transaction_version_str;
  if (value.GetString(kSyncTransactionVersion, &sync_transaction_version_str) &&
      !base::StringToInt64(sync_transaction_version_str,
                           &sync_transaction_version))
    return false;

  node->set_sync_transaction_version(sync_transaction_version);

  return true;
}

bool BookmarkCodec::DecodeMetaInfo(const base::DictionaryValue& value,
                                   BookmarkNode::MetaInfoMap* meta_info_map,
                                   int64* sync_transaction_version) {
  DCHECK(meta_info_map);
  DCHECK(sync_transaction_version);
  meta_info_map->clear();

  const base::Value* meta_info;
  if (!value.Get(kMetaInfo, &meta_info))
    return true;

  scoped_ptr<base::Value> deserialized_holder;

  // Meta info used to be stored as a serialized dictionary, so attempt to
  // parse the value as one.
  if (meta_info->IsType(base::Value::TYPE_STRING)) {
    std::string meta_info_str;
    meta_info->GetAsString(&meta_info_str);
    JSONStringValueSerializer serializer(meta_info_str);
    deserialized_holder.reset(serializer.Deserialize(NULL, NULL));
    if (!deserialized_holder)
      return false;
    meta_info = deserialized_holder.get();
  }
  // meta_info is now either the kMetaInfo node, or the deserialized node if it
  // was stored as a string. Either way it should now be a (possibly nested)
  // dictionary of meta info values.
  const base::DictionaryValue* meta_info_dict;
  if (!meta_info->GetAsDictionary(&meta_info_dict))
    return false;
  DecodeMetaInfoHelper(*meta_info_dict, std::string(), meta_info_map);

  // Previously sync transaction version was stored in the meta info field
  // using this key. If the key is present when decoding, set the sync
  // transaction version to its value, then delete the field.
  if (deserialized_holder) {
    const char kBookmarkTransactionVersionKey[] = "sync.transaction_version";
    BookmarkNode::MetaInfoMap::iterator it =
        meta_info_map->find(kBookmarkTransactionVersionKey);
    if (it != meta_info_map->end()) {
      base::StringToInt64(it->second, sync_transaction_version);
      meta_info_map->erase(it);
    }
  }

  return true;
}

void BookmarkCodec::DecodeMetaInfoHelper(
    const base::DictionaryValue& dict,
    const std::string& prefix,
    BookmarkNode::MetaInfoMap* meta_info_map) {
  for (base::DictionaryValue::Iterator it(dict); !it.IsAtEnd(); it.Advance()) {
    if (it.value().IsType(base::Value::TYPE_DICTIONARY)) {
      const base::DictionaryValue* subdict;
      it.value().GetAsDictionary(&subdict);
      DecodeMetaInfoHelper(*subdict, prefix + it.key() + ".", meta_info_map);
    } else if (it.value().IsType(base::Value::TYPE_STRING)) {
      it.value().GetAsString(&(*meta_info_map)[prefix + it.key()]);
    }
  }
}

void BookmarkCodec::ReassignIDs(BookmarkNode* bb_node,
                                BookmarkNode* other_node,
                                BookmarkNode* mobile_node) {
  maximum_id_ = 0;
  ReassignIDsHelper(bb_node);
  ReassignIDsHelper(other_node);
  ReassignIDsHelper(mobile_node);
  ids_reassigned_ = true;
}

void BookmarkCodec::ReassignIDsHelper(BookmarkNode* node) {
  DCHECK(node);
  node->set_id(++maximum_id_);
  for (int i = 0; i < node->child_count(); ++i)
    ReassignIDsHelper(node->GetChild(i));
}

void BookmarkCodec::UpdateChecksum(const std::string& str) {
  base::MD5Update(&md5_context_, str);
}

void BookmarkCodec::UpdateChecksum(const base::string16& str) {
  base::MD5Update(&md5_context_,
                  base::StringPiece(
                      reinterpret_cast<const char*>(str.data()),
                      str.length() * sizeof(str[0])));
}

void BookmarkCodec::UpdateChecksumWithUrlNode(const std::string& id,
                                              const base::string16& title,
                                              const std::string& url) {
  DCHECK(base::IsStringUTF8(url));
  UpdateChecksum(id);
  UpdateChecksum(title);
  UpdateChecksum(kTypeURL);
  UpdateChecksum(url);
}

void BookmarkCodec::UpdateChecksumWithFolderNode(const std::string& id,
                                                 const base::string16& title) {
  UpdateChecksum(id);
  UpdateChecksum(title);
  UpdateChecksum(kTypeFolder);
}

void BookmarkCodec::InitializeChecksum() {
  base::MD5Init(&md5_context_);
}

void BookmarkCodec::FinalizeChecksum() {
  base::MD5Digest digest;
  base::MD5Final(&digest, &md5_context_);
  computed_checksum_ = base::MD5DigestToBase16(digest);
}
