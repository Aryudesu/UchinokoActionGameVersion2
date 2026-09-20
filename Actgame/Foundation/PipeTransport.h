#pragma once

#include "CharacterController.h"
#include "Coordinates.h"

#include <string>
#include <vector>

namespace uchinoko {

enum class PipeDirection {
	Down,
	Up,
	Right,
	Left
};

struct PipeLink {
	std::string Id;
	// プレイヤー左上座標。tiles単位の外部データからworld座標へ変換して保持する。
	WorldPosition EntryPosition;
	PipeDirection EnterDirection = PipeDirection::Down;

	// "." は同一ステージ。別値はシーン側が解釈してステージ切替に利用する。
	std::string TargetStage = ".";
	// 出現完了後のプレイヤー左上座標。
	WorldPosition ExitPosition;
	// 土管から「出てくる」方向。
	PipeDirection ExitDirection = PipeDirection::Up;
};

class PipeNetwork {
public:
	bool Add(const PipeLink& Link);
	const std::vector<PipeLink>& Links() const { return Links_; }

private:
	std::vector<PipeLink> Links_;
};

enum class PipeTransportPhase {
	Idle,
	Entering,
	WaitingForTransfer,
	Emerging
};

struct PipeTransferRequest {
	std::string LinkId;
	std::string TargetStage;
	WorldPosition ExitPosition;
	PipeDirection ExitDirection = PipeDirection::Up;
};

class PipeTransport {
public:
	static constexpr int TransitionFrames = 32;

	void Reset();

	bool IsActive() const { return Phase_ != PipeTransportPhase::Idle; }
	PipeTransportPhase Phase() const { return Phase_; }
	int Frame() const { return Frame_; }

	// V1の入力方向付き土管進入に相当。
	// EntryPositionからTolerance以内で、リンクと同方向の入力がある時だけ開始する。
	bool TryBegin(
		const CharacterInput& Input,
		const CharacterController& Player,
		const PipeNetwork& Network,
		float Tolerance = 4.0f);

	// Entering/Emerging中に1px/frameで強制移動する。
	// WaitingForTransferでは何もしない。
	void Update(CharacterController& Player);

	bool HasTransferRequest() const { return TransferPending_; }
	const PipeTransferRequest& TransferRequest() const { return TransferRequest_; }

	// シーン切替または同一ステージ内ワープ完了後に呼ぶ。
	// ExitPositionの1タイル奥から32フレームかけて出現する。
	void BeginEmergence(
		CharacterController& Player,
		WorldPosition ExitPosition,
		PipeDirection ExitDirection);

	static WorldPosition DirectionVector(PipeDirection Direction);
	static bool MatchesInput(PipeDirection Direction, const CharacterInput& Input);

private:
	PipeTransportPhase Phase_ = PipeTransportPhase::Idle;
	int Frame_ = 0;
	bool HasCurrentLink_ = false;
	PipeLink CurrentLink_;
	bool TransferPending_ = false;
	PipeTransferRequest TransferRequest_;
	WorldPosition EmergenceTarget_;
	PipeDirection EmergenceDirection_ = PipeDirection::Up;
};

} // namespace uchinoko
