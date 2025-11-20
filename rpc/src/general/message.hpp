#pragma once
#include "abstract.hpp"
#include "detail.hpp"
#include "fields.hpp"
#include "publicconfig.hpp"

namespace lcz_rpc
{
    //Json消息基类
    class JsonMessage:public BaseMessage
    {
        public:
        using ptr = std::shared_ptr<JsonMessage>;  
        // 序列化消息
        virtual std::string serialize()override
        {
            std::string output;
            bool ret=JSON::serialize(_data,output);
            if(!ret)
            {
                ELOG("Serialize failed!");
                return "";
            }
            return output;
        }
        // 反序列化消息
        virtual bool unserialize(const std::string &msg)override
        {
            return JSON::deserialize(msg,_data);
        }
        // JsonMessage 默认视为合法
        virtual bool check()override
        {
            return true;
        }
        
    protected:
        Json::Value _data;       // 消息数据（改为 protected，让子类可以访问）
    
    };
    //Json请求消息
    class JsonRequest:public JsonMessage
    {
        public:
        using ptr = std::shared_ptr<JsonRequest>;  
        
        // 通用方法：获取和设置方法名
        std::string method() const
        {
            return _data[KEY_METHOD].asString();
        }
        void setMethod(const std::string &method)
        {
            _data[KEY_METHOD] = method;
        }
    };
    //Json响应消息
    class JsonResponse:public JsonMessage
    {
        public:
        using ptr = std::shared_ptr<JsonResponse>; 
        // 检查消息有效性 主要验证响应码和结果
        virtual bool check()override
        {
           if(!_data.isMember(KEY_RCODE))
           {
                ELOG("Response code is not found!");
                return false;
           }
           if(!_data[KEY_RCODE].isIntegral())
           {
                ELOG("Response code is not integral!");
                return false;
           }
           return true;
        }
        
        // 通用方法：获取和设置响应码
        RespCode rcode() const
        {
            return static_cast<RespCode>(_data[KEY_RCODE].asInt());
        }
        void setRcode(RespCode rcode)
        {
            _data[KEY_RCODE] = static_cast<int>(rcode);
        }
        
        // 通用方法：获取和设置结果
        Json::Value result() const
        {
            return _data[KEY_RESULT];
        }
        void setResult(const Json::Value &result)
        {
            _data[KEY_RESULT] = result;
        }
    };
    //Rpc请求消息
    class RpcRequest:public JsonRequest
    {
        public:
        using ptr = std::shared_ptr<RpcRequest>;  
        virtual bool check()override
        {
            //长度 消息类型 id长度 id data
            //     Rpcrequest
            if(_data[KEY_METHOD].isString()==false/*消息类型不为字符串*/||
            _data[KEY_METHOD].isNull()/*消息类型不能为空*/)
            {
               ELOG("Method is not string or null!");
                return false;
            }
            if(_data[KEY_PARAMS].isObject()==false/*参数类型错误*/||
            _data[KEY_PARAMS].isNull()/*参数不能为空*/)
            {
                ELOG("Params is not object or null!");
                return false;
            }
            return true;
        }
        //作为rpc请求的消息，可以设置参数
        Json::Value params()const
        {
            return _data[KEY_PARAMS];//获取参数
        }
        void setParams(const Json::Value &params)
        {
                _data[KEY_PARAMS] = params;
        }
    };
    //Rpc响应消息
    class RpcResponse:public JsonResponse
    {
        public:
        using ptr = std::shared_ptr<RpcResponse>; 
        virtual bool check()override
        {
            //对于响应消息，响应码不能为空，结果不能为空
            if(_data[KEY_RCODE].isIntegral()==false||
            _data[KEY_RCODE].isNull())
            {
                ELOG("Response code is not integral or null!");
                return false;
            }
            if(_data[KEY_RESULT].isNull())
            {
                ELOG("Result is null!");
                return false;
            }
            return true;
        }
    };
    //主题请求消息
    class TopicRequest:public JsonRequest
    {
        public:
        using ptr = std::shared_ptr<TopicRequest>; 
        virtual bool check()override
        {           
            if(_data[KEY_TOPIC_KEY].isString()==false||
            _data[KEY_TOPIC_KEY].isNull())
            {
                ELOG("主题键不是字符串或为空!");
                return false;
            }
            if(_data[KEY_OPTYPE].isIntegral()==false||
            _data[KEY_OPTYPE].isNull())
            {
                ELOG("参数不是对象或为空!");
                return false;
            }
            if(_data[KEY_OPTYPE].asInt()==static_cast<int>(TopicOpType::PUBLISH)&&(_data[KEY_TOPIC_MSG].isString()==false||
            _data[KEY_TOPIC_MSG].isNull()))
            {
                ELOG("主题消息不是字符串或为空!");
                return false;
            }
            switch (forwardStrategy()) {
                case TopicForwardStrategy::FANOUT:
                    if (fanoutLimit() <= 0) 
                    {
                        ELOG("扇出数量限制不能小于等于0!");
                        return false;
                    }
                    break;
                case TopicForwardStrategy::SOURCE_HASH:
                    if (shardKey().empty()) 
                    {
                        ELOG("源哈希键不能为空!");
                        return false;
                    }
                    break;
                case TopicForwardStrategy::PRIORITY:
                    if (priority() <= 0 && tags().empty()) 
                    {
                        ELOG("优先级不能小于等于0且标签不能为空!");
                        return false;
                    }
                    break;
                case TopicForwardStrategy::REDUNDANT:
                    if (redundantCount() <= 1) 
                    {
                        ELOG("冗余投递数量不能小于等于1!");
                        return false;
                    }
                    break;
                default:
                    break;
                }
            return true;
        } 
        //获取当前使用的转发策略
        TopicForwardStrategy forwardStrategy()const{return static_cast<TopicForwardStrategy>(_data.get(KEY_TOPIC_FORWARD, static_cast<int>(TopicForwardStrategy::BROADCAST)).asInt());}
        //设置当前使用的转发策略
        void setForwardStrategy(TopicForwardStrategy forwardStrategy){ _data[KEY_TOPIC_FORWARD] = static_cast<int>(forwardStrategy);}
        //获取扇出数量限制
        int fanoutLimit()const
        {
            return _data.get(KEY_TOPIC_FANOUT,0).asInt();//安全获取扇出数量限制
        }
        //设置扇出数量限制
        void setFanoutLimit(int fanoutLimit)
        {
           _data[KEY_TOPIC_FANOUT] = fanoutLimit;
        }
        //获取源哈希键
        const std::string shardKey()const
        {
            return _data.get(KEY_TOPIC_SHARD_KEY,"").asString();//安全获取源哈希键
        }
        //设置源哈希键
        void setShardKey(const std::string &shardKey)
        {
            _data[KEY_TOPIC_SHARD_KEY] = shardKey;
        }
        //获取优先级
        int priority()const
        {
            return _data.get(KEY_TOPIC_PRIORITY,0).asInt();//安全获取优先级
        }
        //设置优先级
        void setPriority(int priority)
        {
            _data[KEY_TOPIC_PRIORITY] = priority;
        }
        //获取标签
        // std::string tags()const
        // {
        //     return _data.get(KEY_TOPIC_TAGS,"").asString();//安全获取标签
        // }
        std::vector<std::string> tags() const 
        {
            std::vector<std::string> result;
            const auto &arr = _data.get(KEY_TOPIC_TAGS,Json::arrayValue);
            if (!arr.isArray()) return result;
            result.reserve(arr.size());
            for (const auto &item : arr)
            if (item.isString()) result.push_back(item.asString());
            return result;
        }
        void setTags(const std::vector<std::string> &tags)
        {
            _data[KEY_TOPIC_TAGS] = Json::Value(Json::arrayValue);
            for(const auto &tag : tags)
            {
                _data[KEY_TOPIC_TAGS].append(tag);
            }
        }
        //获取冗余投递数量
        int redundantCount()const
        {
            return _data.get(KEY_TOPIC_REDUNDANT,0).asInt();//安全获取冗余投递数量
        }
        //设置冗余投递数量
        void setRedundantCount(int redundantCount)
        {
            _data[KEY_TOPIC_REDUNDANT] = redundantCount;
        }
        //提供获取和设置主题key的方法
        std::string topicKey()const
        {
            return _data[KEY_TOPIC_KEY].asString();
        }
        void setTopicKey(const std::string &topicKey)
        {
            _data[KEY_TOPIC_KEY] = topicKey;
        }
        //提供操作类型获取和设置的方法
        TopicOpType optype()const
        {
            return static_cast<TopicOpType>(_data[KEY_OPTYPE].asInt());
        }
        void setOptype(TopicOpType optype)
        {
            _data[KEY_OPTYPE] = static_cast<int>(optype);
        }
        //提供主题消息获取和设置的方法
        std::string topicMsg()const
        {
            return _data[KEY_TOPIC_MSG].asString();
        }
        void setTopicMsg(const std::string &topicMsg)
        {
            _data[KEY_TOPIC_MSG] = topicMsg;
        }
    };
    //主题响应消息
    class TopicResponse:public JsonResponse
    {
        public: 
        using ptr = std::shared_ptr<TopicResponse>;  
        //这里不重写check()方法，因为主题响应消息的响应码和结果都是可选的
        // rcode, setRcode, result, setResult 继承自 JsonResponse
    };
    // //在lcz_rpc命名空间定义的HostInfoDetail结构体，用于存储主机信息和负载信息
    // struct HostInfoDetail {
    //     std::string ip;
    //     int port;
    //     int load=0; // 新增
    //     HostInfoDetail() = default;
    //     HostInfoDetail(const std::string &ip_, int port_, int load_)
    //         : ip(ip_), port(port_), load(load_) {}
    // };
    //服务请求消息
    class ServiceRequest:public JsonRequest
    {
        public: 
        using ptr = std::shared_ptr<ServiceRequest>; 
        int load()const{
            return _data.get(KEY_LOAD,0).asInt();//安全获取负载信息
        }
        void setLoad(int load)
        {
            _data[KEY_LOAD] = load;
        }
        virtual bool check()override
        {
           
            //对于服务请求，方法名不能为空，操作类型不能为空，主机信息不能为空
            if(_data[KEY_METHOD].isString()==false||
            _data[KEY_METHOD].isNull())
            {
               ELOG("Method is not string or null!");
                return false;
            }
            if(_data[KEY_OPTYPE].isIntegral()==false||
            _data[KEY_OPTYPE].isNull())
            {
               ELOG("Op type is not integral or null!");
                return false;
            }
            //不是服务发现的话，就需要提供主机信息
            if(_data[KEY_OPTYPE].asInt()!=static_cast<int>(ServiceOpType::DISCOVER)&&
                (_data[KEY_HOST].isObject()==false||
                _data[KEY_HOST].isNull()||
                _data[KEY_HOST][KEY_HOST_IP].isString()==false||
                _data[KEY_HOST][KEY_HOST_IP].isNull()||
                _data[KEY_HOST][KEY_HOST_PORT].isIntegral()==false||
                _data[KEY_HOST][KEY_HOST_PORT].isNull()))
            {
                ELOG("service discover host is not object or null or ip is not string or null or port is not integral or null!");
                return false;
            }
            if(_data[KEY_OPTYPE].asInt()==static_cast<int>(ServiceOpType::LOAD_REPORT)&&
            (_data[KEY_LOAD].isIntegral()==false||
            _data[KEY_LOAD].isNull()))
            {
                ELOG("没有上报负载信息!");
                return false;
            }
            
            return true;
        } 
        // method, setMethod 继承自 JsonRequest
        //提供获取和设置操作类型的方法
        ServiceOpType optype()const
        {
            return static_cast<ServiceOpType>(_data[KEY_OPTYPE].asInt());
        }
        void setOptype(ServiceOpType optype)
        {
            _data[KEY_OPTYPE] = static_cast<int>(optype);
        }
        //提供获取和设置主机信息的方法
        HostInfo host()const
        {
            return std::make_pair(_data[KEY_HOST][KEY_HOST_IP].asString(),_data[KEY_HOST][KEY_HOST_PORT].asInt());
        }
        void setHost(const HostInfo &host)
        {
            _data[KEY_HOST] = Json::Value(Json::objectValue);
            _data[KEY_HOST][KEY_HOST_IP] = host.first;
            _data[KEY_HOST][KEY_HOST_PORT] = host.second;
        }
    };
    //服务响应消息
    class ServiceResponse:public JsonResponse
    {
        public: 
        using ptr = std::shared_ptr<ServiceResponse>;  
        virtual bool check()override
        {
            //对于服务响应消息，响应码不能为空，操作类型不能为空，如果操作类型不是发现服务，则方法名不能为空，主机信息不能为空
            if(_data[KEY_RCODE].isIntegral()==false||
            _data[KEY_RCODE].isNull())
            {
                ELOG("Response code is not integral or null!");
                return false;
            }
            if(_data[KEY_OPTYPE].isIntegral()==false||
            _data[KEY_OPTYPE].isNull())
            {
                ELOG("Op type is not integral or null!");
                return false;
            }
            if(_data[KEY_OPTYPE].asInt()==static_cast<int>(ServiceOpType::DISCOVER)&&
            (   _data[KEY_METHOD].isString()==false||
                _data[KEY_METHOD].isNull()||
                _data[KEY_HOST].isArray()==false||
                _data[KEY_HOST].isNull()))
            {
               ELOG("service discover method is not string or null or host is not array or null!");
                return false;
            }
            return true;
        }
        // rcode, setRcode, result, setResult 继承自 JsonResponse
        // 但 ServiceResponse 还需要 method 和 optype
        
        std::string method() const
        {
            return _data[KEY_METHOD].asString();
        }
        void setMethod(const std::string &method)
        {
            _data[KEY_METHOD] = method;
        }
        
        void setOptype(ServiceOpType optype)
        {
            _data[KEY_OPTYPE] = static_cast<int>(optype);
        }
        ServiceOpType optype()const
        {
            return static_cast<ServiceOpType>(_data[KEY_OPTYPE].asInt());
        }
        //提供获取和设置主机信息的方法 - 服务发现
       //获取主机列表（从 JSON 数组格式读取）
        std::vector<HostInfo> hosts() const
        {
            std::vector<HostInfo> addresses;
            
            for(int i = 0; i < _data[KEY_HOST].size(); i++)
            {
                addresses.emplace_back(
                    _data[KEY_HOST][i][KEY_HOST_IP].asString(),
                    _data[KEY_HOST][i][KEY_HOST_PORT].asInt()
                );
            }
            return addresses;
        }
         // 设置主机列表（保存为 JSON 数组格式）
         void setHost(const std::vector<HostInfo> &addresses)
         {
             _data[KEY_HOST] = Json::Value(Json::arrayValue);
             
             for(const auto &address : addresses)
             {
                 Json::Value hostObj(Json::objectValue);
                 hostObj[KEY_HOST_IP] = address.first;
                 hostObj[KEY_HOST_PORT] = address.second;
                 _data[KEY_HOST].append(hostObj);
             }
         }
         //添加负载上报后的获取负载均衡后的主机详情方法
        std::vector<HostDetail> hostsDetail() const
        {
            std::vector<HostDetail> hostsdetails;
            
            for(int i = 0; i < _data[KEY_HOST].size(); i++)
            {
                HostInfo host(_data[KEY_HOST][i][KEY_HOST_IP].asString(),
                             _data[KEY_HOST][i][KEY_HOST_PORT].asInt());
                int load = _data[KEY_HOST][i].get(KEY_LOAD,0).asInt();
                hostsdetails.emplace_back(host, load);
            }
            return hostsdetails;
        }
        void setHostDetails(const std::vector<HostDetail> &addresses) {
            _data[KEY_HOST] = Json::Value(Json::arrayValue);
            for (const auto &detail : addresses) {
                Json::Value hostObj(Json::objectValue);
                hostObj[KEY_HOST_IP] = detail.host.first;
                hostObj[KEY_HOST_PORT] = detail.host.second;
                hostObj[KEY_LOAD] = detail.load;
                _data[KEY_HOST].append(hostObj);
            }
        }

       

    };

    //实现消息对象的创建工厂
    class MessageFactory
    {
        public:
        //创建消息对象
        static BaseMessage::ptr create(MsgType msgtype)
        {
            BaseMessage::ptr msg;
            switch(msgtype)
            {
                case MsgType::REQ_RPC:
                    msg = std::make_shared<RpcRequest>();
                    break;
                case MsgType::RSP_RPC:
                    msg = std::make_shared<RpcResponse>();
                    break;
                case MsgType::REQ_TOPIC:
                    msg = std::make_shared<TopicRequest>();
                    break;
                case MsgType::RSP_TOPIC:
                    msg = std::make_shared<TopicResponse>();
                    break;
                case MsgType::REQ_SERVICE:
                    msg = std::make_shared<ServiceRequest>();
                    break;
                case MsgType::RSP_SERVICE:
                    msg = std::make_shared<ServiceResponse>();
                    break;
                default:
                    ELOG("Invalid message type!");
                    return nullptr;
            }
            // 自动设置消息类型，确保 msgType 正确
            if (msg) {
                msg->setMsgType(msgtype);
            }
            return msg;
        }
        template<typename TYPE,typename... ARGS>
        static typename TYPE::ptr create(ARGS&&... args)
        {
            return std::make_shared<TYPE>(std::forward<ARGS>(args)...);
        }

    };
}
