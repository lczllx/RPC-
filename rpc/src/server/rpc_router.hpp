#pragma once
#include "../general/net.hpp"
#include "../general/message.hpp"
#include "../general/dispacher.hpp"
#include "../general/publicconfig.hpp"

/*服务端对rpc请求的处理
1. 接收RPC请求 → 2. 根据method名查找服务 → 3. 参数校验
   ↓
4. 调用服务方法 → 5. 处理业务逻辑 → 6. 返回响应结果
*/
namespace lcz_rpc{
    namespace server{
        enum class ValType {
            BOOL = 0,       // 布尔值: true/false
            INTEGRAL,       // 整型: int8, int16, int32, int64 等
            NUMERIC,        // 数值型: float, double 等
            STRING,         // 字符串: std::string
            ARRAY,          // 数组: 同类型元素集合
            OBJECT,         // 对象: 键值对集合
            NULL_TYPE       // 6: 空值
            
        };
        //服务描述类
        // 描述单个 RPC 方法：校验参数、执行回调、校验返回值
        class ServiceDescribe
        {
            public:
            using ptr=std::shared_ptr<ServiceDescribe>;
            using ParamsDescribe=std::pair<std::string,ValType>;
            using ServiceCallback=std::function<void(const Json::Value& ,Json::Value& )>;
            ServiceDescribe(std::string&& method_name,ServiceCallback&& cb, std::vector<ParamsDescribe>&& params_desc,ValType return_type)
            :_method_name(std::move(method_name)),_service_cb(std::move(cb)),_params_desc(std::move(params_desc)),_return_type(return_type){}
            
            bool checkParams(const Json::Value& params)
            {
                for(auto& desc:_params_desc)
                {
                    if(params.isMember(desc.first)==false)
                    {
                        ELOG("字段 %s 校验失败",desc.first.c_str());
                        return false;
                    }
                    if(check(desc.second,params[desc.first])==false)
                    {
                        ELOG("类型 %s 校验失败",desc.first.c_str());
                        return false;
                    }
                }
                return true;
            }
            bool call(const Json::Value& param,Json::Value& result)
            {
                _service_cb(param,result);
                if(check_return_ty(result)==false)
                {
                    ELOG("回调 函数中的处理结果校验失败");
                    return false;
                }
                return true;
            }
            const std::string& getMethodname()const {return _method_name;}
            private:
            bool check_return_ty(const Json::Value& val)
            {
                return check(_return_type,val);
            }
            bool check(ValType valtype,const Json::Value& val)
            {
                switch(valtype)
                {
                    case ValType::BOOL: return val.isBool();
                    case ValType::INTEGRAL: return val.isIntegral();
                    case ValType::NUMERIC: return val.isNumeric();
                    case ValType::STRING: return val.isString();
                    case ValType::ARRAY: return val.isArray();
                    case ValType::OBJECT: return val.isObject();
                    case ValType::NULL_TYPE: return val.isNull();
                }
                return false;
            }
            private:    
            std::string _method_name; 
            ServiceCallback _service_cb;
            std::vector<ParamsDescribe> _params_desc;
            ValType _return_type;
        };
        //建造者模式
        class ServiceFactory
        {
            public:
            void setReturntype(ValType rtype){_return_type=rtype;}
            void setMethodName(const std::string& method_name){_method_name=method_name;}
            void setParamdescribe(const std::string& param_name,ValType vtype){_params_desc.emplace_back(param_name,vtype);}
            void setServiceCallback(const ServiceDescribe::ServiceCallback& cb){_service_cb=cb;}
            
            // ServiceDescribe::ptr build(){return std::make_shared<ServiceDescribe>(std::move(_method_name),std::move(_service_cb),std::move(_params_desc),_return_type);}
            ServiceDescribe::ptr build()
            {
                std::string method_name = _method_name;
                ServiceDescribe::ServiceCallback service_cb = _service_cb;
                std::vector<ServiceDescribe::ParamsDescribe> params_desc = _params_desc;
                
                return std::make_shared<ServiceDescribe>(
                    std::move(method_name),
                    std::move(service_cb), 
                    std::move(params_desc),
                    _return_type
                );
            }
            private:
            std::string _method_name; 
            ServiceDescribe::ServiceCallback _service_cb;
            std::vector<ServiceDescribe::ParamsDescribe> _params_desc;
            ValType _return_type;

        };
        //服务管理类
        // 简单的线程安全注册表（method -> ServiceDescribe）
        class ServiceManager
        {
            public:
            using ptr=std::shared_ptr<ServiceManager>;
            void add(const ServiceDescribe::ptr& service){std::unique_lock<std::mutex> lock(_mutex);_services[service->getMethodname()]=service;}
            
            ServiceDescribe::ptr select(const std::string& methodname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_services.find(methodname);
                if(it!=_services.end())
                {
                    return it->second;
                }
                return nullptr;
            }
            bool remove(const std::string& methodname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_services.find(methodname);
                if(it!=_services.end())
                {
                     _services.erase(it);
                     return true;
                }
                return false;
            }

            private:
            std::mutex _mutex;
            std::unordered_map<std::string,ServiceDescribe::ptr> _services;
        };
        //
        // 核心路由器：将 RPC 请求派发到对应的 ServiceDescribe
        class RpcRouter
        {
            public:
            using ptr=std::shared_ptr<RpcRouter>;
            RpcRouter() : _manager(std::make_shared<ServiceManager>()) {}
            //注册到dispacher模块对rpc请求进行回调处理的业务函数
            void onrpcRequst(const BaseConnection::ptr& conn,RpcRequest::ptr& req)
            {
                DLOG("RpcRouter recv method=%s", req->method().c_str());
                auto service=_manager->select(req->method());
                if(service.get()==nullptr)
                {
                    ELOG("服务不存在,method:%s",req->method().c_str());
                    return response(conn,req,Json::Value(),RespCode::SERVICE_NOT_FOUND);
                }
                if(service->checkParams(req->params())==false)
                {
                     ELOG("参数校验失败,method:%s",req->method().c_str());
                    return response(conn,req,Json::Value(),RespCode::INVALID_PARAMS);
                }
                Json::Value result;
                bool ret=service->call(req->params(),result);
                if(ret==false)
                {
                    ELOG("这里应该是服务调用失败,method:%s",req->method().c_str());
                    return response(conn,req,Json::Value(),RespCode::INTERNAL_ERROR);
                }
                DLOG("RpcRouter respond method=%s", req->method().c_str());
                return response(conn,req,result,RespCode::SUCCESS);
            }
            //提供给用户注册服务
            void registerMethod(const ServiceDescribe::ptr& service){_manager->add(service);}
            void response(const BaseConnection::ptr& conn,const RpcRequest::ptr& req,const Json::Value& result,RespCode rcode)
            {
                auto resp=MessageFactory::create<RpcResponse>();
                resp->setId(req->rid());
                resp->setMsgType(lcz_rpc::MsgType::RSP_RPC);
                resp->setRcode(rcode);
                resp->setResult(result);
                conn->send(resp);
            }
            private:
            ServiceManager::ptr _manager;
        };
    }
}