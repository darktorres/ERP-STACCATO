import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { SessionUser } from '@erp-staccato/shared';

interface AuthState {
  user: SessionUser | null;
  token: string | null;
  isAuthenticated: boolean;

  // Actions
  setAuth: (user: SessionUser, token: string) => void;
  logout: () => void;
}

export const useAuthStore = create<AuthState>()(
  persist(
    (set) => ({
      user: null,
      token: null,
      isAuthenticated: false,

      setAuth: (user, token) => {
        // Store token in localStorage for tRPC client
        localStorage.setItem('auth-token', token);
        set({ user, token, isAuthenticated: true });
      },

      logout: () => {
        localStorage.removeItem('auth-token');
        set({ user: null, token: null, isAuthenticated: false });
      },
    }),
    {
      name: 'auth-storage',
      partialize: (state) => ({
        user: state.user,
        token: state.token,
        isAuthenticated: state.isAuthenticated,
      }),
    },
  ),
);
