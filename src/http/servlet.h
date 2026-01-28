#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "basic/utils.h"
#include "http/http.h"
#include "http/http_session.h"

namespace http {
class Servlet {
public:
  typedef std::shared_ptr<Servlet> ptr;

  Servlet(const std::string& name) : m_name(name) {}

  virtual ~Servlet() {}

  virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                         HttpSession::ptr session) = 0;

  const std::string& getName() const { return m_name; }

protected:
  std::string m_name;
};

class FunctionServlet : public Servlet {
public:
  typedef std::shared_ptr<FunctionServlet> ptr;
  typedef std::function<int32_t(HttpRequest::ptr request, HttpResponse::ptr response,
                                HttpSession::ptr session)>
      callback;

  FunctionServlet(callback cb);
  virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                         HttpSession::ptr session) override;

private:
  callback m_cb;
};

class IServletCreator {
public:
  typedef std::shared_ptr<IServletCreator> ptr;
  virtual ~IServletCreator() {}
  virtual Servlet::ptr get() const     = 0;
  virtual std::string  getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
public:
  typedef std::shared_ptr<HoldServletCreator> ptr;
  HoldServletCreator(Servlet::ptr slt) : m_servlet(slt) {}

  Servlet::ptr get() const override { return m_servlet; }

  std::string getName() const override { return m_servlet->getName(); }

private:
  Servlet::ptr m_servlet;
};

template <class T>
class ServletCreator : public IServletCreator {
public:
  typedef std::shared_ptr<ServletCreator> ptr;

  ServletCreator() {}

  Servlet::ptr get() const override { return Servlet::ptr(new T); }

  std::string getName() const override { return type_name<T>(); }
};

class NotFoundServlet : public Servlet {
public:
  typedef std::shared_ptr<NotFoundServlet> ptr;
  NotFoundServlet(const std::string& name);
  virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                         HttpSession::ptr session) override;

private:
  std::string m_name;
  std::string m_content;
};

class ServletDispatch : public Servlet {
public:
  /// 智能指针类型定义
  typedef std::shared_ptr<ServletDispatch> ptr;
  /// 读写锁类型定义
  typedef RWMutex RWMutexType;

  /**
   * @brief 构造函数
   */
  ServletDispatch();
  virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                         HttpSession::ptr session) override;
  void            addServlet(const std::string& uri, Servlet::ptr slt);
  void            addServlet(const std::string& uri, FunctionServlet::callback cb);
  void            addGlobServlet(const std::string& uri, Servlet::ptr slt);

  void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

  void addServletCreator(const std::string& uri, IServletCreator::ptr creator);
  void addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator);

  template <class T>
  void addServletCreator(const std::string& uri) {
    addServletCreator(uri, std::make_shared<ServletCreator<T> >());
  }

  template <class T>
  void addGlobServletCreator(const std::string& uri) {
    addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
  }

  void delServlet(const std::string& uri);

  void delGlobServlet(const std::string& uri);

  Servlet::ptr getDefault() const { return m_default; }

  void         setDefault(Servlet::ptr v) { m_default = v; }
  Servlet::ptr getServlet(const std::string& uri);
  Servlet::ptr getGlobServlet(const std::string& uri);
  Servlet::ptr getMatchedServlet(const std::string& uri);

  void listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
  void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos);

private:
  /// 读写互斥量
  RWMutexType m_mutex;
  /// 精准匹配servlet MAP
  /// uri(/server/xxx) -> servlet
  std::unordered_map<std::string, IServletCreator::ptr> m_datas;
  /// 模糊匹配servlet 数组
  /// uri(/server/*) -> servlet
  std::vector<std::pair<std::string, IServletCreator::ptr> > m_globs;
  /// 默认servlet，所有路径都没匹配到时使用
  Servlet::ptr m_default;
};

}  // namespace http