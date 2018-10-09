#include "binding.hpp"

namespace carmen {

using namespace v8;

// temporary, to make the signatures of the rocksdb and memory cache versions match
template <class T>
intarray __get(JSCache<T>* jsc, std::string phrase, langfield_type langfield) {
    return jsc->cache.__get(phrase, langfield);
}

// temporary, to make the signatures of the rocksdb and memory cache versions match
template <class T>
intarray __getmatching(JSCache<T>* jsc, std::string phrase, bool match_prefixes, langfield_type langfield) {
    return jsc->cache.__getmatching(phrase, match_prefixes, langfield);
}

template intarray __get<MemoryCache>(JSCache<MemoryCache>*, std::string phrase, langfield_type langfield);
template intarray __get<RocksDBCache>(JSCache<RocksDBCache>*, std::string phrase, langfield_type langfield);
template intarray __getmatching<MemoryCache>(JSCache<MemoryCache>*, std::string phrase, bool match_prefixes, langfield_type langfield);
template intarray __getmatching<RocksDBCache>(JSCache<RocksDBCache>*, std::string phrase, bool match_prefixes, langfield_type langfield);

template <>
void JSCache<RocksDBCache>::Initialize(Handle<Object> target) {
    Nan::HandleScope scope;
    Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(JSCache::New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(Nan::New("RocksDBCache").ToLocalChecked());
    Nan::SetPrototypeMethod(t, "pack", JSRocksDBCache::pack);
    Nan::SetPrototypeMethod(t, "list", JSRocksDBCache::list);
    Nan::SetPrototypeMethod(t, "_get", _get);
    Nan::SetPrototypeMethod(t, "_getMatching", _getmatching);
    Nan::SetMethod(t, "merge", merge);
    target->Set(Nan::New("RocksDBCache").ToLocalChecked(), t->GetFunction());
    constructor.Reset(t);
}

template <>
void JSCache<MemoryCache>::Initialize(Handle<Object> target) {
    Nan::HandleScope scope;
    Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(JSCache::New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(Nan::New("MemoryCache").ToLocalChecked());
    Nan::SetPrototypeMethod(t, "pack", JSMemoryCache::pack);
    Nan::SetPrototypeMethod(t, "list", JSMemoryCache::list);
    Nan::SetPrototypeMethod(t, "_set", _set);
    Nan::SetPrototypeMethod(t, "_get", _get);
    Nan::SetPrototypeMethod(t, "_getMatching", _getmatching);
    target->Set(Nan::New("MemoryCache").ToLocalChecked(), t->GetFunction());
    constructor.Reset(t);
}

template <class T>
JSCache<T>::JSCache()
    : ObjectWrap(),
      cache() {}

template <class T>
JSCache<T>::~JSCache() {}

/**
 * Writes an identical copy JSCache from another JSCache; not really used
 *
 * @name pack
 * @memberof JSCache
 * @param {String}, filename
 * @returns {Boolean}
 * @example
 * const cache = require('@mapbox/carmen-cache');
 * const JSCache = new cache.JSCache('a');
 *
 * cache.pack('filename');
 *
 */

template <class T>
NAN_METHOD(JSCache<T>::pack) {
    if (info.Length() < 1) {
        return Nan::ThrowTypeError("expected one info: 'filename'");
    }
    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("first argument must be a String");
    }
    try {
        Nan::Utf8String utf8_filename(info[0]);
        if (utf8_filename.length() < 1) {
            return Nan::ThrowTypeError("first arg must be a String");
        }
        std::string filename(*utf8_filename);

        T* c = &(node::ObjectWrap::Unwrap<JSCache<T>>(info.This())->cache);

        // TODO: move into wrapped class and throw a c++-style exception, to be
        // caught here
        // if (c->db && c->db->GetName() == filename) {
        //     return Nan::ThrowTypeError("rocksdb file is already loaded read-only; unload first");
        // } else {
            c->pack(filename);
        // }
        info.GetReturnValue().Set(true);
        return;
    } catch (std::exception const& ex) {
        return Nan::ThrowTypeError(ex.what());
    }
}

/**
 * lists the keys in the JSCache object
 *
 * @name list
 * @memberof JSCache
 * @param {String} id
 * @returns {Array} Set of keys/ids
 * @example
 * const cache = require('@mapbox/carmen-cache');
 * const JSCache = new cache.JSCache('a');
 *
 * cache.list('a', (err, result) => {
 *    if (err) throw err;
 *    console.log(result);
 * });
 *
 */

template <class T>
NAN_METHOD(JSCache<T>::list) {
    try {
        T* c = &(node::ObjectWrap::Unwrap<JSCache<T>>(info.This())->cache);
        Local<Array> ids = Nan::New<Array>();

        std::vector<std::pair<std::string, langfield_type>> results = c->list();

        unsigned idx = 0;
        for (auto const& tuple: results) {
            Local<Array> out = Nan::New<Array>();
            out->Set(0, Nan::New(tuple.first).ToLocalChecked());

            langfield_type langfield = tuple.second;
            if (langfield == ALL_LANGUAGES) {
                out->Set(1, Nan::Null());
            } else {
                out->Set(1, langfieldToLangarray(langfield));
            }

            ids->Set(idx++, out);
        }

        info.GetReturnValue().Set(ids);
        return;
    } catch (std::exception const& ex) {
        return Nan::ThrowTypeError(ex.what());
    }
}

/**
 * merges the contents from 2 JSCaches
 *
 * @name merge
 * @memberof JSCache
 * @param {String} JSCache file 1
 * @param {String} JSCache file 2
 * @param {String} result JSCache file
 * @param {String} method which is either concat or freq
 * @param {Function} callback called from the mergeAfter method
 * @returns {Set} Set of ids
 * @example
 * const cache = require('@mapbox/carmen-cache');
 * const JSCache = new cache.JSCache('a');
 *
 * cache.merge('file1', 'file2', 'resultFile', 'method', (err, result) => {
 *    if (err) throw err;
 *    console.log(result);
 * });
 *
 */

template <>
NAN_METHOD(JSCache<RocksDBCache>::merge) {
    if (!info[0]->IsString()) return Nan::ThrowTypeError("argument 1 must be a String (infile 1)");
    if (!info[1]->IsString()) return Nan::ThrowTypeError("argument 2 must be a String (infile 2)");
    if (!info[2]->IsString()) return Nan::ThrowTypeError("argument 3 must be a String (outfile)");
    if (!info[3]->IsString()) return Nan::ThrowTypeError("argument 4 must be a String (method)");
    if (!info[4]->IsFunction()) return Nan::ThrowTypeError("argument 5 must be a callback function");

    std::string in1 = *Nan::Utf8String(info[0]->ToString());
    std::string in2 = *Nan::Utf8String(info[1]->ToString());
    std::string out = *Nan::Utf8String(info[2]->ToString());
    Local<Value> callback = info[4];
    std::string method = *Nan::Utf8String(info[3]->ToString());

    MergeBaton* baton = new MergeBaton();
    baton->filename1 = in1;
    baton->filename2 = in2;
    baton->filename3 = out;
    baton->method = method;
    baton->callback.Reset(callback.As<Function>());
    baton->request.data = baton;
    uv_queue_work(uv_default_loop(), &baton->request, mergeQueue, static_cast<uv_after_work_cb>(mergeAfter));
    info.GetReturnValue().Set(Nan::Undefined());
    return;
}

/**
* Creates an in-memory key-value store mapping phrases  and language IDs
* to lists of corresponding grids (grids ie are integer representations of occurrences of the phrase within an index)
*
 * @name JSCache
 * @memberof JSCache
 * @param {String} id
 * @param {String} filename
 * @returns {Object}
 * @example
 * const cache = require('@mapbox/carmen-cache');
 * const JSCache = new cache.JSCache('a', 'filename');
 *
 */

template <>
NAN_METHOD(JSCache<RocksDBCache>::New) {
    if (!info.IsConstructCall()) {
        return Nan::ThrowTypeError("Cannot call constructor as function, you need to use 'new' keyword");
    }
    try {
        if (info.Length() < 2) {
            return Nan::ThrowTypeError("expected arguments 'id' and 'filename'");
        }
        if (!info[0]->IsString()) {
            return Nan::ThrowTypeError("first argument 'id' must be a String");
        }
        if (!info[1]->IsString()) {
            return Nan::ThrowTypeError("second argument 'filename' must be a String");
        }

        Nan::Utf8String utf8_filename(info[1]);
        if (utf8_filename.length() < 1) {
            return Nan::ThrowTypeError("second arg must be a String");
        }
        std::string filename(*utf8_filename);

        JSCache<RocksDBCache>* im = new JSCache<RocksDBCache>();
        im->cache = RocksDBCache(filename);
        im->Wrap(info.This());
        info.This()->Set(Nan::New("id").ToLocalChecked(), info[0]);
        info.GetReturnValue().Set(info.This());
        return;
    } catch (std::exception const& ex) {
        return Nan::ThrowTypeError(ex.what());
    }
}


/**
 * Creates an in-memory key-value store mapping phrases  and language IDs
 * to lists of corresponding grids (grids ie are integer representations of occurrences of the phrase within an index)
 *
 * @name MemoryCache
 * @memberof MemoryCache
 * @param {String} id
 * @returns {Array} grid of integers
 * @example
 * const cache = require('@mapbox/carmen-cache');
 * const MemoryCache = new cache.MemoryCache(id, languages);
 *
 */

template <>
NAN_METHOD(JSCache<MemoryCache>::New) {
    if (!info.IsConstructCall()) {
        return Nan::ThrowTypeError("Cannot call constructor as function, you need to use 'new' keyword");
    }
    try {
        if (info.Length() < 1) {
            return Nan::ThrowTypeError("expected 'id' argument");
        }
        if (!info[0]->IsString()) {
            return Nan::ThrowTypeError("first argument 'id' must be a String");
        }

        JSCache<MemoryCache>* im = new JSCache<MemoryCache>();
        im->cache = MemoryCache();
        im->Wrap(info.This());
        info.This()->Set(Nan::New("id").ToLocalChecked(), info[0]);
        info.GetReturnValue().Set(info.This());
        return;
    } catch (std::exception const& ex) {
        return Nan::ThrowTypeError(ex.what());
    }
}

 /**
  * Retrieves data exactly matching phrase and language settings by id
  *
  * @name get
  * @memberof JSCache
  * @param {String} id
  * @param {Boolean} matches_prefixes: T if it matches exactly, F: if it does not
  * @param {Array} optional; array of languages
  * @returns {Array} integers referring to grids
  * @example
  * const cache = require('@mapbox/carmen-cache');
  * const JSCache = new cache.JSCache('a');
  *
  * JSCache.get(id, languages);
  *  // => [grid, grid, grid, grid... ]
  *
  */

 template <class T>
 NAN_METHOD(JSCache<T>::_get) {
     return _genericget<JSCache>(info);
 }

/**
 * Retrieves grid that at least partially matches phrase and/or language inputs
 *
 * @name get
 * @memberof JSCache
 * @param {String} id
 * @param {Boolean} matches_prefixes: T if it matches exactly, F: if it does not
 * @param {Array} optional; array of languages
 * @returns {Array} integers referring to grids
 * @example
 * const cache = require('@mapbox/carmen-cache');
 * const JSCache = new cache.JSCache('a');
 *
 * JSCache.get(id, languages);
 *  // => [grid, grid, grid, grid... ]
 *
 */

template <class T>
NAN_METHOD(JSCache<T>::_getmatching) {
    return _genericgetmatching<JSCache<T>>(info);
}

template <>
NAN_METHOD(JSCache<MemoryCache>::_set) {
    if (info.Length() < 2) {
        return Nan::ThrowTypeError("expected at least two info: id, data, [languages], [append]");
    }
    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("first arg must be a String");
    }
    if (!info[1]->IsArray()) {
        return Nan::ThrowTypeError("second arg must be an Array");
    }
    Local<Array> data = Local<Array>::Cast(info[1]);
    if (data->IsNull() || data->IsUndefined()) {
        return Nan::ThrowTypeError("an array expected for second argument");
    }
    try {

        Nan::Utf8String utf8_id(info[0]);
        if (utf8_id.length() < 1) {
            return Nan::ThrowTypeError("first arg must be a String");
        }
        std::string id(*utf8_id);

        langfield_type langfield;
        if (info.Length() > 2 && !(info[2]->IsNull() || info[2]->IsUndefined())) {
            if (!info[2]->IsArray()) {
                return Nan::ThrowTypeError("third arg, if supplied must be an Array");
            }
            langfield = langarrayToLangfield(Local<Array>::Cast(info[2]));
        } else {
            langfield = ALL_LANGUAGES;
        }

        bool append = info.Length() > 3 && info[3]->IsBoolean() && info[3]->BooleanValue();

        unsigned array_size = data->Length();
        auto vec_data = intarray();
        vec_data.reserve(array_size);

        for (unsigned i = 0; i < array_size; ++i) {
            vec_data.emplace_back(static_cast<uint64_t>(data->Get(i)->NumberValue()));
        }

        MemoryCache* c = &(node::ObjectWrap::Unwrap<JSMemoryCache>(info.This())->cache);
        c->_set(id, vec_data, langfield, append);
    } catch (std::exception const& ex) {
        return Nan::ThrowTypeError(ex.what());
    }
    info.GetReturnValue().Set(Nan::Undefined());
    return;
}

extern "C" {
static void start(Handle<Object> target) {
    JSMemoryCache::Initialize(target);
    JSRocksDBCache::Initialize(target);
    NormalizationCache::Initialize(target);
    Nan::SetMethod(target, "coalesce", coalesce);
}
}

} // namespace carmen

// this macro expansion includes an old-style cast and is beyond our control
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
NODE_MODULE(carmen, carmen::start)
#pragma clang diagnostic pop
