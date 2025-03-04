/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "RemoteDecoderModule.h"

#include "base/thread.h"
#include "mozilla/dom/ContentChild.h"  // for launching RDD w/ ContentChild
#include "mozilla/layers/SynchronousTask.h"
#include "mozilla/StaticPrefs.h"
#include "mozilla/SyncRunnable.h"

#ifdef MOZ_AV1
#  include "AOMDecoder.h"
#endif
#include "RemoteAudioDecoder.h"
#include "RemoteDecoderManagerChild.h"
#include "RemoteMediaDataDecoder.h"
#include "RemoteVideoDecoder.h"
#include "VorbisDecoder.h"

namespace mozilla {

using base::Thread;
using dom::ContentChild;
using namespace ipc;
using namespace layers;

RemoteDecoderModule::RemoteDecoderModule()
    : mManagerThread(RemoteDecoderManagerChild::GetManagerThread()) {}

bool RemoteDecoderModule::SupportsMimeType(
    const nsACString& aMimeType, DecoderDoctorDiagnostics* aDiagnostics) const {
  bool supports = false;

#ifdef MOZ_AV1
  if (StaticPrefs::MediaAv1Enabled()) {
    supports |= AOMDecoder::IsAV1(aMimeType);
  }
#endif
  if (StaticPrefs::MediaRddVorbisEnabled()) {
    supports |= VorbisDataDecoder::IsVorbis(aMimeType);
  }

  MOZ_LOG(
      sPDMLog, LogLevel::Debug,
      ("Sandbox decoder %s requested type", supports ? "supports" : "rejects"));
  return supports;
}

void RemoteDecoderModule::LaunchRDDProcessIfNeeded() {
  if (!XRE_IsContentProcess()) {
    return;
  }

  // We have a couple possible states here.  We are in a content process
  // and:
  // 1) the RDD process has never been launched.  RDD should be launched
  //    and the IPC connections setup.
  // 2) the RDD process has been launched, but this particular content
  //    process has not setup (or has lost) its IPC connection.
  // In the code below, we assume we need to launch the RDD process and
  // setup the IPC connections.  However, if the manager thread for
  // RemoteDecoderManagerChild is available we do a quick check to see
  // if we can send (meaning the IPC channel is open).  If we can send,
  // then no work is necessary.  If we can't send, then we call
  // LaunchRDDProcess which will launch RDD if necessary, and setup the
  // IPC connections between *this* content process and the RDD process.
  bool needsLaunch = true;
  if (mManagerThread) {
    RefPtr<Runnable> task = NS_NewRunnableFunction(
        "RemoteDecoderModule::LaunchRDDProcessIfNeeded-CheckSend", [&]() {
          if (RemoteDecoderManagerChild::GetRDDProcessSingleton()) {
            needsLaunch =
                !RemoteDecoderManagerChild::GetRDDProcessSingleton()->CanSend();
          }
        });
    SyncRunnable::DispatchToThread(mManagerThread, task);
  }

  if (needsLaunch) {
    ContentChild::GetSingleton()->LaunchRDDProcess();
    mManagerThread = RemoteDecoderManagerChild::GetManagerThread();
  }
}

already_AddRefed<MediaDataDecoder> RemoteDecoderModule::CreateAudioDecoder(
    const CreateDecoderParams& aParams) {
  LaunchRDDProcessIfNeeded();

  if (!mManagerThread) {
    return nullptr;
  }

  RefPtr<RemoteAudioDecoderChild> child = new RemoteAudioDecoderChild();
  MediaResult result(NS_OK);
  // We can use child as a ref here because this is a sync dispatch. In
  // the error case for InitIPDL, we can't just let the RefPtr go out of
  // scope at the end of the method because it will release the
  // RemoteAudioDecoderChild on the wrong thread.  This will assert in
  // RemoteDecoderChild's destructor.  Passing the RefPtr by reference
  // allows us to release the RemoteAudioDecoderChild on the manager
  // thread during this single dispatch.
  RefPtr<Runnable> task =
      NS_NewRunnableFunction("RemoteDecoderModule::CreateAudioDecoder", [&]() {
        result = child->InitIPDL(aParams.AudioConfig(), aParams.mOptions);
        if (NS_FAILED(result)) {
          // Release RemoteAudioDecoderChild here, while we're on
          // manager thread.  Don't just let the RefPtr go out of scope.
          child = nullptr;
        }
      });
  SyncRunnable::DispatchToThread(mManagerThread, task);

  if (NS_FAILED(result)) {
    if (aParams.mError) {
      *aParams.mError = result;
    }
    return nullptr;
  }

  RefPtr<RemoteMediaDataDecoder> object = new RemoteMediaDataDecoder(
      child, mManagerThread,
      RemoteDecoderManagerChild::GetManagerAbstractThread());

  return object.forget();
}

already_AddRefed<MediaDataDecoder> RemoteDecoderModule::CreateVideoDecoder(
    const CreateDecoderParams& aParams) {
  LaunchRDDProcessIfNeeded();

  if (!mManagerThread) {
    return nullptr;
  }

  RefPtr<RemoteVideoDecoderChild> child = new RemoteVideoDecoderChild();
  MediaResult result(NS_OK);
  // We can use child as a ref here because this is a sync dispatch. In
  // the error case for InitIPDL, we can't just let the RefPtr go out of
  // scope at the end of the method because it will release the
  // RemoteVideoDecoderChild on the wrong thread.  This will assert in
  // RemoteDecoderChild's destructor.  Passing the RefPtr by reference
  // allows us to release the RemoteVideoDecoderChild on the manager
  // thread during this single dispatch.
  RefPtr<Runnable> task =
      NS_NewRunnableFunction("RemoteDecoderModule::CreateVideoDecoder", [&]() {
        result = child->InitIPDL(aParams.VideoConfig(), aParams.mRate.mValue,
                                 aParams.mOptions);
        if (NS_FAILED(result)) {
          // Release RemoteVideoDecoderChild here, while we're on
          // manager thread.  Don't just let the RefPtr go out of scope.
          child = nullptr;
        }
      });
  SyncRunnable::DispatchToThread(mManagerThread, task);

  if (NS_FAILED(result)) {
    if (aParams.mError) {
      *aParams.mError = result;
    }
    return nullptr;
  }

  RefPtr<RemoteMediaDataDecoder> object = new RemoteMediaDataDecoder(
      child, mManagerThread,
      RemoteDecoderManagerChild::GetManagerAbstractThread());

  return object.forget();
}

}  // namespace mozilla
