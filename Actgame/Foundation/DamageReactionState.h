#pragma once

namespace uchinoko {

// V1 Player::DamageUpdate() の16F被ダメージ状態を、
// HPや描画とは分離して保持する。
class DamageReactionState {
public:
	static constexpr int Version1DurationFrames = 16;

	void Reset() {
		Active_ = false;
		Frame_ = 0;
		KnockbackDirection_ = -1;
	}

	bool Begin(int KnockbackDirection) {
		if (Active_) return false;
		Active_ = true;
		Frame_ = 0;
		KnockbackDirection_ = KnockbackDirection < 0 ? -1 : 1;
		return true;
	}

	// V1は DamageTime++ 後、DamageTime < 16 の間だけ speed.x=3*dire。
	// したがって1～15Fはノックバック、16F目は横速度0で通常状態へ戻る。
	float AdvanceFrame() {
		if (!Active_) return 0.0f;
		++Frame_;
		const float Horizontal =
			Frame_ < Version1DurationFrames
				? static_cast<float>(KnockbackDirection_)
				: 0.0f;
		if (Frame_ >= Version1DurationFrames) {
			Active_ = false;
		}
		return Horizontal;
	}

	bool Active() const { return Active_; }
	int Frame() const { return Frame_; }
	int KnockbackDirection() const { return KnockbackDirection_; }

private:
	bool Active_ = false;
	int Frame_ = 0;
	int KnockbackDirection_ = -1;
};

} // namespace uchinoko
