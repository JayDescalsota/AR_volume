import type { Corner, HeightMode, Measurement, Phase, Vec3 } from '../geometry/types';

export type AppState = {
  phase: Phase;
  corners: Corner[];
  closed: boolean;
  heightMode: HeightMode;
  manualHeight: number;
  heightBase: Vec3 | null;
  heightTop: Vec3 | null;
  roofPoints: Corner[];
  measurement: Measurement | null;
  message: string | null;
};

export const initialState: AppState = {
  phase: 'capturing',
  corners: [],
  closed: false,
  heightMode: 'manual',
  manualHeight: 3,
  heightBase: null,
  heightTop: null,
  roofPoints: [],
  measurement: null,
  message: null,
};

export type Action =
  | { type: 'cornerAdd'; corner: Corner }
  | { type: 'cornerRefine'; id: string; point: Vec3 }
  | { type: 'closeLoop' }
  | { type: 'backToCapture' }
  | { type: 'setHeightMode'; mode: HeightMode }
  | { type: 'setManualHeight'; value: number }
  | { type: 'heightTap'; point: Vec3 }
  | { type: 'roofAdd'; corner: Corner }
  | { type: 'measured'; measurement: Measurement }
  | { type: 'message'; text: string | null }
  | { type: 'reset' };

export function reducer(state: AppState, action: Action): AppState {
  switch (action.type) {
    case 'cornerAdd':
      if (state.closed || state.phase !== 'capturing') return state;
      return { ...state, corners: [...state.corners, action.corner], message: null };

    case 'cornerRefine': {
      let changed = false;
      const corners = state.corners.map(corner => {
        if (corner.id !== action.id) return corner;
        changed = true;
        return { ...corner, point: action.point };
      });
      return changed ? { ...state, corners } : state;
    }

    case 'closeLoop':
      if (state.corners.length < 3) {
        return { ...state, message: 'Add at least three corners before closing.' };
      }
      return { ...state, phase: 'height', closed: true, message: null };

    case 'backToCapture':
      return {
        ...state,
        phase: 'capturing',
        closed: false,
        measurement: null,
        heightBase: null,
        heightTop: null,
        roofPoints: [],
        message: null,
      };

    case 'setHeightMode':
      return {
        ...state,
        heightMode: action.mode,
        heightBase: null,
        heightTop: null,
        roofPoints: [],
        message: null,
      };

    case 'setManualHeight':
      return { ...state, manualHeight: action.value };

    case 'heightTap':
      if (!state.heightBase) return { ...state, heightBase: action.point };
      if (!state.heightTop) return { ...state, heightTop: action.point };
      return { ...state, heightBase: action.point, heightTop: null };

    case 'roofAdd':
      return { ...state, roofPoints: [...state.roofPoints, action.corner] };

    case 'measured':
      return { ...state, phase: 'result', measurement: action.measurement };

    case 'message':
      return { ...state, message: action.text };

    case 'reset':
      return initialState;

    default:
      return state;
  }
}
