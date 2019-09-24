#pragma once

namespace fun {
namespace net {

class IHostIdFactory {
 public:
  virtual ~IHostIdFactory() {}

  virtual HostId Create(double absolute_time, HostId assigned_host_id = HostId_None) = 0;
  virtual void Drop(double absolute_time, HostId id_to_drop) = 0;
  virtual int32 GetRecycleCount(HostId host_id) = 0;
};


class HostIdFactory : public IHostIdFactory {
 public:
  HostIdFactory();

  // IHostIdFactory interface
  HostId Create(double absolute_time, HostId assigned_host_id = HostId_None) override;
  void Drop(double absolute_time, HostId id_to_drop) override;
  int32 GetRecycleCount(HostId host_id) override;

 private:
  HostId num_;
};


class RecycleHostIdFactory : public IHostIdFactory {
 public:
  RecycleHostIdFactory(double issue_valid_time_);

  // IHostIdFactory interface
  HostId Create(double absolute_time, HostId assigned_host_id = HostId_None) override;
  void Drop(double absolute_time, HostId id_to_drop) override;
  int32 GetRecycleCount(HostId host_id) override;

private:
  struct Node {
    HostId host_id;
    int32 recycle_count;
    double dropped_time;

    Node()
      : host_id(HostId_None), recycle_count(0), dropped_time(0) {}
  };

  double issue_valid_time_;

  HostId last_issued_value_;

  Map<HostId, Node> drop_map_;
  List<HostId> drop_list_;

private:
  HostId NewHostId();
};


class AssignHostIdFactory : public IHostIdFactory {
 public:
  AssignHostIdFactory();

  // IHostIdFactory interface
  HostId Create(double absolute_time, HostId assigned_host_id = HostId_None) override;
  void Drop(double absolute_time, HostId id_to_drop) override;
  int32 GetRecycleCount(HostId host_id) override;

 private:
  HostId num_;
  Map<HostId, bool> assigned_map_;

  HostId NewHostId();
};

} // namespace net
} // namespace fun
