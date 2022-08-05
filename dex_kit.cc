#include "dex_kit.h"
#include "file_helper.h"
#include "acdat/Builder.h"
#include "thread_pool.h"
#include "byte_code_util.h"
#include "opcode_util.h"
#include "slicer/dex_bytecode.h"
#include <algorithm>

namespace dexkit {

using namespace acdat;

DexKit::DexKit(std::string_view apk_path) {
    InitStartTime();
    auto map = MemMap(apk_path);
    if (!map.ok()) {
        return;
    }
    auto zip_file = ZipFile::Open(map);
    std::cout << "Open zip file: " << apk_path << std::endl;
    std::cout << "used time: " << GetUsedTime() << std::endl;
    std::cout << "uncompress dex entry" << std::endl;
    if (zip_file) {
        std::vector<std::pair<int, ZipLocalFile *>> dexs;
        for (int idx = 1;; ++idx) {
            auto entry_name = "classes" + (idx == 1 ? std::string() : std::to_string(idx)) + ".dex";
            auto entry = zip_file->Find(entry_name);
            if (!entry) {
                break;
            }
            dexs.emplace_back(idx, entry);
        }
        dex_images_.resize(dexs.size());
        maps_.resize(dexs.size() + 1);
        ThreadPool pool(thread_num_);
        for (auto &dex_pair: dexs) {
            pool.enqueue([&dex_pair, this]() {
                auto dex_image = dex_pair.second->uncompress();
                if (!dex_image.ok()) {
                    return;
                }
                dex_images_[dex_pair.first - 1] = std::make_pair(dex_image.addr(), dex_image.len());
                maps_[dex_pair.first] = std::move(dex_image);
            });
        }
    }
    std::cout << "used time: " << GetUsedTime() << std::endl;
    InitImages();
    maps_.emplace_back(std::move(map));
}

DexKit::DexKit(std::vector<std::pair<const void *, size_t>> &dex_images) {
    InitStartTime();
    InitImages();
}

std::map<std::string_view, std::vector<std::string_view>>
DexKit::LocationClasses(std::map<std::string_view, std::set<std::string_view>> &location_map) {
    auto acdat = AhoCorasickDoubleArrayTrie<std::string>();
    std::map<std::string, std::string> buildMap;
    std::map<std::string_view, std::vector<std::string_view>> result;
    for (auto &[name, str_set]: location_map) {
        for (auto &str: str_set) {
            buildMap[str.data()] = str;
        }
    }
    Builder<std::string>().build(buildMap, &acdat);
    ThreadPool pool(thread_num_);
    std::vector<std::future<std::map<std::string_view, std::vector<std::string_view>>>> futures;
    for (int dex_idx = 0; dex_idx < readers_.size(); ++dex_idx) {
        futures.emplace_back(pool.enqueue([&acdat, &location_map, dex_idx, this]() {
            InitCached(dex_idx);
            auto &method_codes = method_codes_[dex_idx];
            auto &class_method_ids = class_method_ids_[dex_idx];
            auto &type_names = type_names_[dex_idx];
            auto &strings = strings_[dex_idx];

            std::map<std::string_view, std::vector<std::string_view>> result;

            std::map<dex::u4, std::string> string_map;
            for (int index = 0; index < strings.size(); ++index) {
                std::function<void(int, int, std::string)> callback =
                        [&string_map, index](int begin, int end, const std::string &value) -> void {
                            string_map[index] = value;
                        };
                acdat.parseText(strings[index].data(), callback);
            }

            if (string_map.empty()) {
                std::cout << "------ dex: " << dex_idx << " not found string\n";
                return std::map<std::string_view, std::vector<std::string_view>>();
            }
            for (int i = 0; i < type_names.size(); ++i) {
                if (class_method_ids[i].empty()) {
                    continue;
                }
                std::set<std::string> search_set;
                for (auto method_idx: class_method_ids[i]) {
                    auto &code = method_codes[method_idx];
                    if (code == nullptr) {
                        continue;
                    }
                    auto p = code->insns;
                    auto end_p = p + code->insns_size;
                    while (p < end_p) {
                        auto instruction = dex::DecodeInstruction(p);
                        auto ptr = p;
                        auto width = GetBytecodeWidth(ptr++);
                        switch (instruction.opcode) {
                            case 0x1a:
                            case 0x1b: {
                                auto index = ReadShort(ptr);
                                if (string_map.find(index) != string_map.end()) {
                                    search_set.emplace(string_map[index]);
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        p += width;
                    }
                }
                for (auto &[real_class, value_set]: location_map) {
                    std::vector<std::string> vec;
                    std::set_intersection(search_set.begin(), search_set.end(), value_set.begin(), value_set.end(),
                                          std::inserter(vec, vec.begin()));
                    if (vec.size() == value_set.size()) {
                        result[real_class].emplace_back(type_names[i]);
                    }
                }
            }
            return result;
        }));
    }
    for (auto &f: futures) {
        auto r = f.get();
        for (auto &[key, value]: r) {
            for (auto &cls_name: value) {
                result[key].emplace_back(cls_name);
            }
        }
    }
    return result;
}

std::vector<std::string_view> DexKit::FindMethodInvoked(std::string_view method_descriptor) {
    return std::vector<std::string_view>();
}

std::vector<std::string_view> DexKit::FindSubClasses(std::string_view class_name) {
    return std::vector<std::string_view>();
}


void DexKit::InitImages() {
    for (auto &[image, size]: dex_images_) {
        auto reader = dex::Reader((const dex::u1 *) image, size);
        readers_.emplace_back(std::move(reader));
    }
    strings_.reserve(readers_.size());
    type_names_.reserve(readers_.size());
    class_method_ids_.reserve(readers_.size());
    method_codes_.reserve(readers_.size());
    proto_type_list_.reserve(readers_.size());

    init_flags_.resize(readers_.size(), false);
}

std::string DexKit::GetClassDescriptor(std::string class_name) {
    std::replace(class_name.begin(), class_name.end(), '.', '/');
    if (class_name.length() > 0 && class_name[0] != 'L') {
        class_name = "L" + class_name + ";";
    }
    return class_name;
}

void DexKit::InitCached(int dex_idx) {
    if (init_flags_[dex_idx]) return;
    auto &reader = readers_[dex_idx];
    auto &strings = strings_[dex_idx];
    auto &type_names = type_names_[dex_idx];
    auto &class_method_ids = class_method_ids_[dex_idx];
    auto &method_codes = method_codes_[dex_idx];
    auto &proto_type_list = proto_type_list_[dex_idx];

    strings.reserve(reader.StringIds().size());
    type_names.resize(reader.TypeIds().size());
    class_method_ids.resize(reader.TypeIds().size());
    method_codes.resize(reader.MethodIds().size(), nullptr);
    proto_type_list.resize(reader.ProtoIds().size(), nullptr);

    for (int str_idx = 0; str_idx < reader.StringIds().size(); ++str_idx) {
        auto &str = reader.StringIds()[str_idx];
        auto *ptr = reader.dataPtr<dex::u1>(str.string_data_off);
        dex::ReadULeb128(&ptr);
        strings.emplace_back(reinterpret_cast<const char *>(ptr));
    }


    for (int type_idx = 0; type_idx < reader.TypeIds().size(); ++type_idx) {
        auto &type_id = reader.TypeIds()[type_idx];
        type_names[type_idx] = strings[type_id.descriptor_idx];
    }


    for (int idx = 0; idx < reader.ClassDefs().size(); ++idx) {
        auto &class_def = reader.ClassDefs()[idx];
        if (class_def.class_data_off == 0) {
            continue;
        }
        const auto *class_data = reader.dataPtr<dex::u1>(class_def.class_data_off);
        dex::u4 static_fields_size = ReadULeb128(&class_data);
        dex::u4 instance_fields_count = ReadULeb128(&class_data);
        dex::u4 direct_methods_count = ReadULeb128(&class_data);
        dex::u4 virtual_methods_count = ReadULeb128(&class_data);

        auto &methods = class_method_ids[class_def.class_idx];
//        methods.resize(direct_methods_count + virtual_methods_count);

        for (int i = 0; i < static_fields_size; ++i) {
            ReadULeb128(&class_data);
            ReadULeb128(&class_data);
        }

        for (int i = 0; i < instance_fields_count; ++i) {
            ReadULeb128(&class_data);
            ReadULeb128(&class_data);
        }

        for (dex::u4 i = 0, method_idx = 0; i < direct_methods_count; ++i) {
            method_idx += ReadULeb128(&class_data);
            ReadULeb128(&class_data);
            dex::u4 code_off = ReadULeb128(&class_data);
            if (code_off == 0) {
                continue;
            }
            method_codes[method_idx] = reader.dataPtr<const dex::Code>(code_off);
            methods.push_back(method_idx);
        }
        for (dex::u4 i = 0, method_idx = 0; i < virtual_methods_count; ++i) {
            method_idx += ReadULeb128(&class_data);
            ReadULeb128(&class_data);
            dex::u4 code_off = ReadULeb128(&class_data);
            if (code_off == 0) {
                continue;
            }
            method_codes[method_idx] = reader.dataPtr<const dex::Code>(code_off);
            methods.emplace_back(method_idx);
        }
    }
    init_flags_[dex_idx] = true;
}

std::string DexKit::GetMethodDescriptor(int dex_idx, uint32_t method_idx) {
    auto &reader = readers_[dex_idx];
    auto &strings = strings_[dex_idx];
    auto &type_names = type_names_[dex_idx];
    auto &method_id = reader.MethodIds()[method_idx];
    auto &proto_id = reader.ProtoIds()[method_id.proto_idx];
    auto &type_list = proto_type_list_[dex_idx][method_id.proto_idx];
    if (type_list == nullptr) {
        if (proto_id.parameters_off == 0) {
            type_list = new dex::TypeList();
        } else {
            type_list = reader.dataPtr<dex::TypeList>(proto_id.parameters_off);
        }
    }
    std::string descriptor(type_names[method_id.class_idx]);
    descriptor += "->";
    descriptor += strings[method_id.name_idx];
    descriptor += '(';
    for (int i = 0; i < type_list->size; ++i) {
        if (i > 0) {
            descriptor += ',';
        }
        descriptor += strings[reader.TypeIds()[type_list->list[i].type_idx].descriptor_idx];
    }
    descriptor += ')';
    descriptor += strings[reader.TypeIds()[proto_id.return_type_idx].descriptor_idx];
    return descriptor;
}

void DexKit::InitStartTime() {
    auto now = std::chrono::system_clock::now();
    start_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

long DexKit::GetUsedTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return now_time - start_time_;
}

}