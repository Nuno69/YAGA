// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	void TurnPlayerBody(zCModel* model, float angle) {
		model->vobTrans.RotateXZ(angle);
	}
}