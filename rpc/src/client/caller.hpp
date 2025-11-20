#pragma once
#include "requestor.hpp"
#include<future>
#include<functional>
#include "../general/publicconfig.hpp"

namespace lcz_rpc
{
    namespace client
    {
        //requestor里面的send是对basemessage进行处理
        //这里的caller是对rpcresponse里面的result进行处理        
        // 封装 Requestor 的常用调用方式：同步 / future / 回调
        class RpcCaller
        {
            public:
            using ptr=std::shared_ptr<RpcCaller>;
            using RpcAsyncRespose=std::future<Json::Value>;
            using ResponseCallback=std::function<void(const Json::Value&)>;
            RpcCaller(const Requestor::ptr& reqtor):_requestor(reqtor){}
            // 同步调用：阻塞等待响应，返回结果 Json::Value
            bool call(const BaseConnection::ptr& conn,const std::string& method_name,const Json::Value& params,Json::Value& result)
            {
                DLOG("RpcCaller sync call method=%s", method_name.c_str());
                RpcRequest::ptr req_msg=MessageFactory::create<RpcRequest>();
                req_msg->setId(uuid());
                req_msg->setMsgType(MsgType::REQ_RPC);
                req_msg->setMethod(method_name);
                req_msg->setParams(params);
                BaseMessage::ptr resp_msg;
                bool ret= _requestor->send(conn,std::dynamic_pointer_cast<BaseMessage>(req_msg),resp_msg);
                if(!ret){ELOG("rpc同步请求失败");return false;}
                RpcResponse::ptr rpc_respmsg=std::dynamic_pointer_cast<RpcResponse>(resp_msg);
                if(rpc_respmsg.get()==nullptr)
                {
                ELOG("类型向下转换失败失败");return false; 
                }
                if(rpc_respmsg->rcode()!=RespCode::SUCCESS)
                {
                    ELOG("rpc请求出错：%s",errReason(rpc_respmsg->rcode()).c_str());return false; 
                }
                result=rpc_respmsg->result();
                DLOG("RpcCaller sync call finish method=%s", method_name.c_str());
                return true;
            }
            // 异步调用：返回 std::future，由调用者自行等待
            bool call(const BaseConnection::ptr& conn, const std::string& method_name,Json::Value& params,RpcAsyncRespose& result)
            {
                DLOG("RpcCaller future call method=%s", method_name.c_str());
                //向服务端发送异步回调请求，设置回调函数，在回调 函数中对pomise设置数据
                auto req_msg=MessageFactory::create<RpcRequest>();
                req_msg->setId(uuid());
                req_msg->setMsgType(MsgType::REQ_RPC);
                req_msg->setMethod(method_name);
                req_msg->setParams(params);
                BaseMessage::ptr resp_msg;
                
                auto json_pomise=std::make_shared<std::promise<Json::Value>>();//防止作用域结束销毁
                result=json_pomise->get_future();///创建 Promise-Future 对，通过 get_future() 连接

                Requestor::ReqCallback cb=std::bind(&RpcCaller::callBack,this,json_pomise,std::placeholders::_1);
                bool ret= _requestor->send(conn,std::dynamic_pointer_cast<BaseMessage>(req_msg),cb);
                if(!ret){ELOG("rpc异步请求失败");return false;}

                return true;

            }
            // 回调模式：响应到达时触发用户提供的回调函数
            bool call(const BaseConnection::ptr& conn, const std::string& method_name,Json::Value& params,const ResponseCallback& cb)
            {
                DLOG("RpcCaller callback call method=%s", method_name.c_str());
                auto req_msg=MessageFactory::create<RpcRequest>();
                req_msg->setId(uuid());
                req_msg->setMsgType(MsgType::REQ_RPC);
                req_msg->setMethod(method_name);
                req_msg->setParams(params);
                BaseMessage::ptr resp_msg;
               
                Requestor::ReqCallback reqcb=std::bind(&RpcCaller::callBackself,this,cb,std::placeholders::_1);
                bool ret= _requestor->send(conn,std::dynamic_pointer_cast<BaseMessage>(req_msg),reqcb);
                if(!ret){ELOG("rpc回调请求失败");return false;}

                return true;
            }
            private:
            // future 模式下的回调：校验响应并设置 promise
            void callBack(std::shared_ptr<std::promise<Json::Value>> result,const BaseMessage::ptr& msg)
            {
                RpcResponse::ptr rpc_respmsg=std::dynamic_pointer_cast<RpcResponse>(msg);
                if(rpc_respmsg.get()==nullptr)
                {
                ELOG("类型向下转换失败失败");return ; 
                }
                if(rpc_respmsg->rcode()!=RespCode::SUCCESS)
                {
                    ELOG("rpc异步出错：%s",errReason(rpc_respmsg->rcode()).c_str());return; 
                }
                result->set_value(rpc_respmsg->result());//被触发时设置结果
            }
            // 回调模式：校验响应后，执行用户提供的 cb
            void callBackself(const ResponseCallback &cb,const BaseMessage::ptr& msg)
            {
                RpcResponse::ptr rpc_respmsg=std::dynamic_pointer_cast<RpcResponse>(msg);
                if(rpc_respmsg.get()==nullptr)
                {
                ELOG("类型向下转换失败失败");return ; 
                }
                if(rpc_respmsg->rcode()!=RespCode::SUCCESS)
                {
                    ELOG("rpc回调出错：%s",errReason(rpc_respmsg->rcode()).c_str());return; 
                }
                cb(rpc_respmsg->result());//使用回调处理结果
            }
            private:
            Requestor::ptr _requestor;
        };
    }
}