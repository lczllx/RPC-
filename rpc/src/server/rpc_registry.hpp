#pragma once
#include "../general/net.hpp"
#include "../general/message.hpp"
#include "../general/dispacher.hpp"
#include <set>
#include "../general/publicconfig.hpp"

namespace lcz_rpc
{
    namespace server
    {
        class ProviderManager
        {
            public:
            using ptr=std::shared_ptr<ProviderManager>;
            struct Provider
            {
                using ptr=std::shared_ptr<Provider>;
                std::mutex mutex;
                int load;
                std::vector<std::string> methods;
                BaseConnection::ptr conn;
                HostInfo address;
                std::chrono::steady_clock::time_point lastheartbeat;//最后心跳时间
                Provider(const BaseConnection::ptr& connection,const HostInfo& host)
                    :conn(connection),address(host),load(0),
                     lastheartbeat(std::chrono::steady_clock::now()){}
                void appendmethod(const std::string& method)
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    methods.emplace_back(method);
                }
            };
            void addProvider(const BaseConnection::ptr& conn,const HostInfo& host,const std::string& method,int load)
            {
                Provider::ptr provider;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it=_connwithp.find(conn);
                    if(it==_connwithp.end())
                    {
                        provider =std::make_shared<Provider>(conn,host);  
                        _connwithp[conn]=provider;           
                    }else{
                        provider=it->second;
                    }
                    _methodwithproviders[method].insert(provider);
                    provider->load=load;
                    provider->lastheartbeat=std::chrono::steady_clock::now();
                }
                    provider->appendmethod(method);
            }
            Provider::ptr getProvider(const BaseConnection::ptr& conn)
            {                
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_connwithp.find(conn);
                if(it==_connwithp.end())
                {
                    return Provider::ptr();
                }else{
                    return it->second;
                }               
            }
            void delProvider(const BaseConnection::ptr& conn)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_connwithp.find(conn);
                if(it==_connwithp.end()){return ;}
                for(auto& method:it->second->methods)
                {
                    _methodwithproviders[method].erase(it->second);
                }
                _connwithp.erase(it);               
            }
            std::vector<HostInfo> methodHost(const std::string& method)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                std::vector<HostInfo>ret;
                auto it=_methodwithproviders.find(method);
                if(it==_methodwithproviders.end())return ret;
                for(auto& provider:it->second)
                {
                    ret.emplace_back(provider->address);
                }
                return ret;
            }
            //添加负载均衡后的主机详情
            std::vector<HostDetail> methodHostDetails(const std::string& method) {
                std::unique_lock<std::mutex> lock(_mutex);
                std::vector<HostDetail> ret;
                auto it = _methodwithproviders.find(method);
                if (it == _methodwithproviders.end()) return ret;
                for (auto &provider : it->second) {
                    HostDetail detail;
                    detail.host.first = provider->address.first;
                    detail.host.second = provider->address.second;
                    detail.load = provider->load;
                    ret.emplace_back(detail);
                }
                return ret;
            }
            //给定 `method + host` 找到已注册的 provider，更新 负载
            bool updateProviderLoad(const std::string &method,
                const HostInfo &host,
                int load)
            {
                    std::unique_lock<std::mutex>lock(_mutex);
                    auto it=_methodwithproviders.find(method);
                    if(it==_methodwithproviders.end()){return false;}
                    for(auto&provider:it->second)
                    {
                        if(provider->address==host){
                            provider->load=load;
                            provider->lastheartbeat=std::chrono::steady_clock::now();
                            return true;
                        }
                    }
                    return false;
            }
            //provider更新最后活跃时间
            bool updateProviderLastHeartbeat(const std::string& method, const HostInfo& host) {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _methodwithproviders.find(method);
                if (it == _methodwithproviders.end()) return false;
                for (auto &p : it->second) {
                    if (p->address == host) {
                        p->lastheartbeat = std::chrono::steady_clock::now();
                        ILOG("%s:%d 心跳检测 更新最后心跳时间", p->address.first.c_str(),p->address.second);
                        return true; 
                    }
                }
                return false;
            }
            //provider超时检测 删除并返回需要广播下线的 (method, host)
            std::vector<std::pair<std::string, HostInfo>> sweepExpired(std::chrono::seconds idle_timeout)
            {
                std::vector<std::pair<std::string, HostInfo>> expired;
                auto now = std::chrono::steady_clock::now();
                std::unique_lock<std::mutex> lock(_mutex);

                for (auto &kv : _methodwithproviders)
                {
                    const std::string &method = kv.first;
                    auto &providers = kv.second;
                    std::vector<Provider::ptr> to_remove_providers;
                    for (auto &p : providers)
                    {
                        auto idle_sec = std::chrono::duration_cast<std::chrono::seconds>(now - p->lastheartbeat).count();
                        if (now - p->lastheartbeat > idle_timeout)
                        {
                            ILOG("[Provider扫描] 发现过期提供者 method=%s %s:%d 闲置时间=%ld秒", 
                                 method.c_str(), p->address.first.c_str(), p->address.second, idle_sec);
                            expired.emplace_back(method, p->address);
                            to_remove_providers.push_back(p);
                        }
                    }
                    for (auto &p : to_remove_providers) providers.erase(p);
                }
                return expired;
            }

            private:
            std::mutex _mutex;
            std::unordered_map<std::string,std::set<Provider::ptr>> _methodwithproviders;
            std::unordered_map<BaseConnection::ptr,Provider::ptr> _connwithp;
        };
        class DiscoverManager
        {
            public:
            using ptr=std::shared_ptr<DiscoverManager>;
            struct Discoverer
            {
                using ptr=std::shared_ptr<Discoverer>;
                std::mutex mutex;
                std::vector<std::string> methods;//发现过的服务名称
                BaseConnection::ptr conn;
                Discoverer(const BaseConnection::ptr& connection):conn(connection){}
                void appendmethod(const std::string& method)
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    methods.emplace_back(method);
                }
            };
            //添加discoverer
            Discoverer::ptr addDiscoverer(const BaseConnection::ptr& conn,const HostInfo& host,const std::string& method)
            {
                Discoverer::ptr discoverer;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it=_connwithd.find(conn);
                    if(it==_connwithd.end())
                    {
                        discoverer =std::make_shared<Discoverer>(conn);  
                        _connwithd[conn]=discoverer;           
                    }else{
                        discoverer=it->second;
                    }
                    _methodwithdiscoverer[method].insert(discoverer);
                }
                    discoverer->appendmethod(method);
                    return discoverer;
            }
            //获取discoverer
            Discoverer::ptr getProvider(const BaseConnection::ptr& conn)
            {                
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_connwithd.find(conn);
                if(it==_connwithd.end())
                {
                    return Discoverer::ptr();
                }else{
                    return it->second;
                }               
            }
            //删除discoverer
            void delProvider(const BaseConnection::ptr& conn)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_connwithd.find(conn);
                if(it==_connwithd.end()){return ;}
                for(auto& method:it->second->methods)
                {
                    _methodwithdiscoverer[method].erase(it->second);
                }
                _connwithd.erase(it);               
            }
            //当有新的服务提供者上线，进⾏上线通知
            void onlineNotify(const std::string& method,const HostInfo& host)
            {
                return notify(method,host,ServiceOpType::ONLINE);
            }
            //当服务提供者下线，进⾏下线通知
            void offlineNotify(const std::string& method,const HostInfo& host)
            {
                return notify(method,host,ServiceOpType::OFFLINE);
            }
            private:
            // 将服务上线/下线事件广播给所有正在等待该 method 的发现者
            void notify(const std::string& method,const HostInfo& host,ServiceOpType service_type)
            {
                std::unique_lock<std::mutex>lock(_mutex);
                auto it=_methodwithdiscoverer.find(method);
                if(it==_methodwithdiscoverer.end()){return ;}
                auto rpc_msg=MessageFactory::create<ServiceRequest>();
                rpc_msg->setHost(host);
                rpc_msg->setId(uuid());
                rpc_msg->setMethod(method);
                rpc_msg->setMsgType(MsgType::REQ_SERVICE);
                rpc_msg->setOptype(service_type);
                
                for(auto& provider:it->second)
                {
                    provider->conn->send(rpc_msg);//通知派发
                }

            }
            std::mutex _mutex;
            std::unordered_map<std::string,std::set<Discoverer::ptr>> _methodwithdiscoverer;
            std::unordered_map<BaseConnection::ptr,Discoverer::ptr> _connwithd;
        };

        class PwithDManager
        {
            public:
            using ptr=std::shared_ptr<PwithDManager>;
            PwithDManager():_provider(std::make_shared<ProviderManager>()),_discoverer(std::make_shared<DiscoverManager>()){}
            void onserviceRequest(const BaseConnection::ptr& conn,const ServiceRequest::ptr& msg)
            {
                ServiceOpType optype=msg->optype();
                if(optype==ServiceOpType::REGISTER)
                {//服务注册通知
                    ILOG("%s:%d 注册服务 %s", msg->host().first.c_str(),msg->host().second, msg->method().c_str());
                    _provider->addProvider(conn,msg->host(),msg->method(),msg->load());//注册服务
                    _discoverer->onlineNotify(msg->method(),msg->host());
                    //后续在这里处理负载均衡
                    return registryResponse(conn,msg);
                }
                else if(optype==ServiceOpType::DISCOVER)
                {//服务发现通知
                    ILOG("客⼾端要进⾏ %s 服务发现！", msg->method().c_str());
                    _discoverer->addDiscoverer(conn,msg->host(),msg->method());
                    return discoverResponse(conn,msg);
                }
                else if(optype==ServiceOpType::LOAD_REPORT)
                {//服务负载上报
                    ILOG("%s:%d 上报负载 %d", msg->host().first.c_str(),msg->host().second, msg->load());
                    //更新负载
                    bool update_success=_provider->updateProviderLoad(msg->method(),msg->host(),msg->load());
                   
                    return updateloadResponse(conn,msg,update_success);
                }
                else if(optype==ServiceOpType::HEARTBEAT_PROVIDER)//提供者向注册中心周期报活，服务端刷新 Provider.lasttime
                {//心跳检测
                    ILOG("[RegistryServer-Provider心跳接收] method=%s %s:%d", 
                         msg->method().c_str(), msg->host().first.c_str(), msg->host().second);
                    bool update_success=_provider->updateProviderLastHeartbeat(msg->method(),msg->host());
                    if (update_success) {
                        ILOG("[RegistryServer-Provider心跳处理] method=%s %s:%d 更新成功", 
                             msg->method().c_str(), msg->host().first.c_str(), msg->host().second);
                    } else {
                        WLOG("[RegistryServer-Provider心跳处理] method=%s %s:%d 更新失败，提供者不存在", 
                             msg->method().c_str(), msg->host().first.c_str(), msg->host().second);
                    }
                    return heartbeatResponse(conn,msg,update_success,ServiceOpType::HEARTBEAT_PROVIDER);
                }
                else{
                    ELOG("收到服务操作请求，但是操作类型错误");
                    return errResponse(conn,msg);
                }
            }
            //连接关闭
            void onconnShoutdown(const BaseConnection::ptr& conn)
            {
                auto provider=_provider->getProvider(conn);
                if(provider.get()!=nullptr)
                {//是服务提供者下线
                    ILOG("%s:%d 服务下线", provider->address.first.c_str(),provider->address.second);
                    for(auto &method:provider->methods)
                    {
                        _discoverer->offlineNotify(method,provider->address);
                    }
                    _provider->delProvider(conn);

                }
                _discoverer->delProvider(conn);
            }
            //定时清理超时服务提供者并通知发现者
            std::vector<std::pair<std::string, HostInfo>> sweepAndNotify(int idle_timeout_sec) {
                auto expired = _provider->sweepExpired(std::chrono::seconds(idle_timeout_sec));
                for (auto &pr : expired) {
                    _discoverer->offlineNotify(pr.first, pr.second);
                }
                return expired;
            }
            private:
            //错误响应
            void errResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr& msg)
            {
                auto msg_resp=MessageFactory::create<ServiceResponse>();
                msg_resp->setId(msg->rid());
                msg_resp->setRcode(RespCode::INVALID_OPTYPE);
                msg_resp->setMsgType(MsgType::RSP_SERVICE);
                msg_resp->setOptype(ServiceOpType::UNKNOWN);
                conn->send(msg_resp);

            }
            //服务注册响应
            void registryResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr& msg)
            {
                auto msg_resp=MessageFactory::create<ServiceResponse>();
                msg_resp->setId(msg->rid());
                msg_resp->setRcode(RespCode::SUCCESS);
                msg_resp->setMsgType(MsgType::RSP_SERVICE);
                msg_resp->setOptype(ServiceOpType::REGISTER);
                conn->send(msg_resp);
            }
            //服务发现响应
            void discoverResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr& msg)
            {
                auto msg_resp=MessageFactory::create<ServiceResponse>();
                msg_resp->setId(msg->rid());
                msg_resp->setRcode(RespCode::SUCCESS);
                msg_resp->setMsgType(MsgType::RSP_SERVICE);
                msg_resp->setOptype(ServiceOpType::DISCOVER);
                msg_resp->setHost(_provider->methodHost(msg->method()));
                auto hosts = _provider->methodHostDetails(msg->method());
                msg_resp->setHostDetails(hosts);
                conn->send(msg_resp);
            }
            //负载上报响应
            void updateloadResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr& msg,bool update_success)
            {
               
                auto msg_resp=MessageFactory::create<ServiceResponse>();
                if(!update_success){
                    ELOG("load report failed: %s %s:%d 未找到对应服务",
                        msg->method().c_str(),
                        msg->host().first.c_str(),
                        msg->host().second);
                   msg_resp->setRcode(RespCode::SERVICE_NOT_FOUND);
                }
                else{
                    msg_resp->setRcode(RespCode::SUCCESS);
                }
                msg_resp->setId(msg->rid());
                msg_resp->setMsgType(MsgType::RSP_SERVICE);
                msg_resp->setOptype(ServiceOpType::LOAD_REPORT);
                conn->send(msg_resp);
            }
            //心跳检测响应
            void heartbeatResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr& msg,bool update_success,ServiceOpType optype)
            {
                auto msg_resp=MessageFactory::create<ServiceResponse>();
                if(!update_success){
                    ELOG("心跳检测失败: %s %s:%d 未找到对应服务提供者",
                        msg->method().c_str(),
                        msg->host().first.c_str(),
                        msg->host().second);
                    msg_resp->setRcode(RespCode::SERVICE_NOT_FOUND);
                }
                else{
                    msg_resp->setRcode(RespCode::SUCCESS);
                }
                msg_resp->setId(msg->rid());
                msg_resp->setMsgType(MsgType::RSP_SERVICE);
                msg_resp->setOptype(optype);
                conn->send(msg_resp);
            }
            private:
            ProviderManager::ptr _provider;
            DiscoverManager::ptr _discoverer;

        };

    } // namespace server
} // namespace lcz_rpc
