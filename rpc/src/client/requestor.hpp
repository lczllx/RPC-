#pragma once

/*对客户端的每一个请求进行管理
发送请求时：创建ReqDescribe并存入映射表
等待响应时：根据请求类型使用不同机制等待结果
收到响应时：通过onResponse查找对应的请求描述，执行相应处理
清理资源：处理完成后删除请求描述，防止内存泄漏
*/
#include "../general/net.hpp"
#include "../general/message.hpp"
#include "../general/dispacher.hpp"
#include<future>
#include<functional>
#include "../general/publicconfig.hpp"

namespace lcz_rpc
{
    namespace client
    {
        class Requestor
        {
            public:
            using ptr=std::shared_ptr<Requestor>;
            using ReqCallback=std::function<void(const BaseMessage::ptr&)>;
            using AsyncResponse=std::future<BaseMessage::ptr>;
            // 单个 RPC 请求的描述信息：记录请求类型、回调以及等待中的 promise
            struct ReqDescribe
            {
                using ptr=std::shared_ptr<ReqDescribe>;
                ReqType reqtype;
                ReqCallback callback;
                std::promise<BaseMessage::ptr> response;
                BaseMessage::ptr request;
            };
            // 处理服务端响应：匹配请求 id，触发对应的 promise 或回调
            void onResponse(const BaseConnection::ptr& conn,BaseMessage::ptr& msg)
            {
                std::string id=msg->rid();
                ReqDescribe::ptr req_desc=getDesc(id);
                if(req_desc.get()==nullptr)
                {
                    ELOG("收到 %s 响应，但消息描述不存在",id.c_str());
                    return;
                }
                if(req_desc->reqtype==ReqType::ASYNC)
                {
                    req_desc->response.set_value(msg);//设置结果
                }
                else if(req_desc->reqtype==ReqType::CALLBACK)
                {
                    if(req_desc->callback)req_desc->callback(msg);//回调处理
                }
                else{
                    ELOG("未知请求类型");
                }
                delDesc(id);//处理完删除掉这个描述信息
            }
            //异步
            bool send(const BaseConnection::ptr& conn,const BaseMessage::ptr& req,AsyncResponse& async_resp)
            {
                ReqDescribe::ptr req_desc=newDesc(req,ReqType::ASYNC);
                if(req_desc.get()==nullptr)
                {
                    ELOG("构造请求描述对象失败！");
                    return false;
                }
                conn->send(req);//异步请求发送
                async_resp= req_desc->response.get_future();//获取关联的future对象
                return true;
            }
            //同步
            bool send(const BaseConnection::ptr& conn,const BaseMessage::ptr& req,BaseMessage::ptr& resp)
            {
                DLOG("Requestor sync send id=%s", req->rid().c_str());
                AsyncResponse async_resp;
                if(send(conn,req,async_resp)==false)
                {
                    ELOG("Requestor sync send failed id=%s", req->rid().c_str());
                    return false;
                }
                resp=async_resp.get();
                DLOG("Requestor sync recv id=%s", req->rid().c_str());
                return true;

            }
            //回调
            bool send(const BaseConnection::ptr& conn,const BaseMessage::ptr& req,const ReqCallback& cb)
            {
                ReqDescribe::ptr req_desc=newDesc(req,ReqType::CALLBACK,cb);
                if(req_desc.get()==nullptr)
                {
                    ELOG("构造请求描述对象失败！");
                    return false;
                }
                conn->send(req);
                return true;
            }
            private:
            ReqDescribe::ptr newDesc(const BaseMessage::ptr& req,ReqType req_type,const ReqCallback& cb=ReqCallback())
            {
                std::unique_lock<std::mutex> lock(_mutex);
                ReqDescribe::ptr req_desc=std::make_shared<ReqDescribe>();
                req_desc->reqtype=req_type;
                req_desc->request=req;
                if(req_type==ReqType::CALLBACK&&cb)req_desc->callback=cb;
                _request_desc[req->rid()]=req_desc;
                DLOG("newDesc add id=%s", req->rid().c_str());
                return req_desc;
            }
            ReqDescribe::ptr getDesc(std::string& rid)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it=_request_desc.find(rid);
                if(it!=_request_desc.end())
                {
                    return it->second;
                }
                return  ReqDescribe::ptr();               
            }
            void delDesc(std::string& rid)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _request_desc.erase(rid);
            }
            private:
            std::mutex _mutex;
            std::unordered_map<std::string,ReqDescribe::ptr> _request_desc;//rid desc
        };
    }
}