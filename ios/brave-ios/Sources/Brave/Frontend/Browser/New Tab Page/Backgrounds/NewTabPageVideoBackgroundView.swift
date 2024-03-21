// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import AVKit
import BraveCore
import BraveUI
import Preferences
import Shared
import SnapKit
import UIKit

class NewTabPageVideoBackgroundView: UIView {
  var playFinishedEvent: (() -> Void)?
  var played25PercentEvent: (() -> Void)?
  var autoplayFinishedEvent: (() -> Void)?
  var playCancelledEvent: (() -> Void)?
  var videoLoadedEvent: ((Bool) -> Void)?

  private let kMaxAutoplayDuration = 6.0

  private var playerLayer = AVPlayerLayer()
  private var playStarted: Bool?
  private var previewAutoplayFinished: Bool = false
  private var timeObserverToken: Any?
  private var isShortVideo = false
  private var playerObserver: NSKeyValueObservation?

  private var videoButtonsView = NewTabPageVideoButtonsView()

  override init(frame: CGRect) {
    super.init(frame: frame)

    playerLayer.frame = frame
    layer.addSublayer(playerLayer)

    addSubview(videoButtonsView)
    videoButtonsView.isHidden = true

    videoButtonsView.tappedBackground = { [weak self] in
      self?.playOrPauseNTTVideo()
    }
    videoButtonsView.tappedCancelButton = { [weak self] in
      self?.videoButtonsView.isHidden = true
      self?.cancelPlay()
    }

    videoButtonsView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    self.playerLayer.frame = self.bounds
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  func setupPlayer(backgroundVideoPath: URL, shouldStartAutoplay: Bool) {
    cleanup()

    self.backgroundColor = parseColorFromFilename(filename: backgroundVideoPath.lastPathComponent)

    let stopFrame = parseStopFrameFromFilename(filename: backgroundVideoPath.lastPathComponent)

    let asset: AVAsset = AVAsset(url: backgroundVideoPath)
    let item = AVPlayerItem(asset: asset)
    let player = AVPlayer(playerItem: item)
    playerLayer.player = player

    if shouldResizeToFill(filename: backgroundVideoPath.lastPathComponent) {
      playerLayer.videoGravity = .resizeAspectFill
    } else {
      playerLayer.videoGravity = .resizeAspect
    }

    if !shouldStartAutoplay {
      previewAutoplayFinished = true
    }

    playerObserver = player.observe(
      \.status,
      options: [.new, .old],
      changeHandler: { [weak self] (player, change) in
        if player.status == .readyToPlay {
          self?.loadVideoTrackParams(
            asset: asset,
            stopFrame: stopFrame,
            shouldStartAutoplay: shouldStartAutoplay
          )
        } else if player.status == .failed {
          self?.videoLoaded(succeeded: false)
        }
      }
    )
  }

  func cancelPlayAndHidePlayerLayer() {
    if !previewAutoplayFinished {
      previewAutoplayFinished = true
      playerLayer.player?.pause()
      autoplayFinished()
    }
    if playStarted != nil {
      cancelPlay()
    }
    self.playerLayer.isHidden = true
    videoButtonsView.isHidden = true
  }

  func showPlayerLayer() {
    self.playerLayer.isHidden = false
  }

  private func loadVideoTrackParams(asset: AVAsset, stopFrame: Int?, shouldStartAutoplay: Bool) {
    Task {
      var frameRate: Float?
      var duration: CMTime?

      if let videoTrack = try? await asset.loadTracks(withMediaType: .video).first {
        frameRate = try? await videoTrack.load(.nominalFrameRate)
        duration = try? await asset.load(.duration)
      }

      DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in
        self?.videoTrackParamsLoaded(
          duration: duration,
          frameRate: frameRate,
          stopFrame: stopFrame,
          shouldStartAutoplay: shouldStartAutoplay
        )
      }
    }
  }

  private func videoTrackParamsLoaded(
    duration: CMTime?,
    frameRate: Float?,
    stopFrame: Int?,
    shouldStartAutoplay: Bool
  ) {
    guard let duration = duration else {
      videoLoaded(succeeded: false)
      return
    }
    videoLoaded(succeeded: true)

    if shouldStartAutoplay {
      startAutoplay(duration: duration, frameRate: frameRate, stopFrame: stopFrame)
    }
  }

  private func startAutoplay(duration: CMTime, frameRate: Float?, stopFrame: Int?) {
    var autoplayLengthSeconds: Float64 = self.kMaxAutoplayDuration
    self.isShortVideo = CMTimeGetSeconds(duration) <= self.kMaxAutoplayDuration

    if self.isShortVideo {
      autoplayLengthSeconds = CMTimeGetSeconds(duration)
    } else if let frameRate = frameRate,
      let stopFrame = stopFrame
    {
      autoplayLengthSeconds = Float64(stopFrame) / Float64(frameRate)
    }

    if !isShortVideo {
      playerLayer.player?.currentItem?.forwardPlaybackEndTime = CMTime(
        seconds: autoplayLengthSeconds,
        preferredTimescale: CMTimeScale(NSEC_PER_SEC)
      )
      NotificationCenter.default.addObserver(
        self,
        selector: #selector(playerItemTimeJumped(_:)),
        name: .AVPlayerItemTimeJumped,
        object: playerLayer.player?.currentItem
      )
    }

    NotificationCenter.default
      .addObserver(
        self,
        selector: #selector(self.playerDidFinishPlaying),
        name: .AVPlayerItemDidPlayToEndTime,
        object: playerLayer.player?.currentItem
      )

    playerLayer.player?.isMuted = true
    playerLayer.player?.play()
  }

  private func cancelPlay() {
    playerLayer.player?.pause()
    if timeObserverToken != nil {
      playerLayer.player?.removeTimeObserver(timeObserverToken!)
      timeObserverToken = nil
    }
    playStarted = nil

    playCancelledEvent?()
  }

  private func cleanup() {
    isShortVideo = false
    playStarted = nil
    previewAutoplayFinished = false
    if timeObserverToken != nil {
      playerLayer.player?.removeTimeObserver(timeObserverToken!)
      timeObserverToken = nil
    }
    NotificationCenter.default.removeObserver(self)
  }

  @objc func playerItemTimeJumped(_ notification: Notification) {
    if isShortVideo || previewAutoplayFinished {
      return
    }
    guard let playerItem = notification.object as? AVPlayerItem else {
      return
    }

    let currentTime = playerItem.currentTime().seconds

    let forwardEndTime = playerItem.forwardPlaybackEndTime.seconds
    if currentTime >= forwardEndTime {
      autoplayFinished()
    }
  }

  @objc private func playerDidFinishPlaying(note: NSNotification) {
    if !previewAutoplayFinished && isShortVideo {
      autoplayFinished()
      return
    }

    if playStarted == nil {
      return
    }
    playStarted = nil

    self.videoButtonsView.isHidden = true
    playFinishedEvent?()
  }

  private func checkPlayPercentage() {
    if timeObserverToken == nil {
      return
    }

    guard let playerItem = playerLayer.player?.currentItem else {
      return
    }

    let duration = CMTimeGetSeconds(playerItem.duration)
    let currentTime = CMTimeGetSeconds(playerItem.currentTime())

    if currentTime / duration >= 0.25 {
      played25PercentEvent?()

      playerLayer.player?.removeTimeObserver(timeObserverToken!)
      timeObserverToken = nil
    }
  }

  func playOrPauseNTTVideo() {
    if playStarted == nil {
      if !previewAutoplayFinished {
        autoplayFinished()
      }

      self.videoButtonsView.isHidden = false
      self.videoButtonsView.setPlayStarted()

      playerLayer.player?.isMuted = false
      playerLayer.player?.pause()
      playerLayer.player?.seek(to: CMTime.zero)
      playerLayer.player?.currentItem?.forwardPlaybackEndTime = CMTime()

      let interval = CMTime(
        seconds: 0.1,
        preferredTimescale: CMTimeScale(NSEC_PER_SEC)
      )
      timeObserverToken =
        playerLayer.player?.addPeriodicTimeObserver(forInterval: interval, queue: nil) {
          [weak self] time in
          self?.checkPlayPercentage()
        }

      playStarted = false
    }

    if !playStarted! {
      playerLayer.player?.play()
      playStarted = true
    } else {
      playerLayer.player?.pause()
      playStarted = false
    }
  }

  private func videoLoaded(succeeded: Bool) {
    if !succeeded {
      previewAutoplayFinished = true
    }
    videoLoadedEvent?(succeeded)
  }

  private func autoplayFinished() {
    previewAutoplayFinished = true
    autoplayFinishedEvent?()
  }

  private func parseColorFromFilename(filename: String) -> UIColor {
    var color: String?
    if let range = filename.range(of: "\\.RGB[a-fA-F0-9]+\\.", options: .regularExpression) {
      color = filename[range].replacingOccurrences(of: ".RGB", with: "")
        .replacingOccurrences(of: ".", with: "")
    }

    guard let color = color,
      color.count == 6
    else {
      return UIColor.black
    }

    var rgbValue: UInt64 = 0
    Scanner(string: color).scanHexInt64(&rgbValue)

    return UIColor(
      red: CGFloat((rgbValue & 0xFF0000) >> 16) / 255.0,
      green: CGFloat((rgbValue & 0x00FF00) >> 8) / 255.0,
      blue: CGFloat(rgbValue & 0x0000FF) / 255.0,
      alpha: CGFloat(1.0)
    )
  }

  private func parseStopFrameFromFilename(filename: String) -> Int? {
    var stopFrame: Int?
    if let range = filename.range(of: "\\.KF\\d+\\.", options: .regularExpression) {
      let numberString = filename[range].replacingOccurrences(of: ".KF", with: "")
        .replacingOccurrences(of: ".", with: "")
      stopFrame = Int(numberString)
    }
    return stopFrame
  }

  private func shouldResizeToFill(filename: String) -> Bool {
    return filename.range(of: "\\.RTF\\.", options: .regularExpression) != nil
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
