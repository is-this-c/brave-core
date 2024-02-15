/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REMOTE_WORKER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REMOTE_WORKER_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace base {
class SequencedTaskRunner;
}

template <typename T>
class RemoteWorker {
 public:
  explicit RemoteWorker(scoped_refptr<base::SequencedTaskRunner> task_runner)
      : task_runner_(task_runner) {}

  ~RemoteWorker() = default;

  mojo::Remote<T>& remote() { return remote_; }
  mojo::Remote<const T>& remote() const { return remote_; }

  auto operator->() const { return remote_.get(); }

  void reset() { remote_.reset(); }

  template <typename Impl, typename... Args>
    requires(std::is_base_of_v<T, Impl>)
  void BindRemote(Args&&... args) {
    reset();
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(BindRemoteOnWorkerSequence<Impl, Args...>,
                                  remote_.BindNewPipeAndPassReceiver(),
                                  std::forward<Args>(args)...));
  }

 private:
  template <typename Impl, typename... Args>
  static void BindRemoteOnWorkerSequence(
      mojo::PendingReceiver<T> pending_receiver,
      Args&&... args) {
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<Impl>(std::forward<Args>(args)...),
        std::move(pending_receiver));
  }

  mojo::Remote<T> remote_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REMOTE_WORKER_H_
