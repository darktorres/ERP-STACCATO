import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { render, screen, fireEvent, waitFor, cleanup } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import React from 'react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import LoginDialog from './LoginDialog';
import { useAuthStore } from '../stores/auth';
import { trpc, trpcClient } from '../lib/trpc';

/**
 * Integration tests for LoginDialog
 * Tests the full login flow with real API calls to backend
 */
describe('LoginDialog Integration Tests', () => {
  beforeEach(() => {
    // Clear auth store before each test
    useAuthStore.setState({ user: null, token: null });
    // Clear localStorage
    localStorage.clear();
  });

  afterEach(() => {
    // Clean up DOM between tests to prevent element reuse
    cleanup();
  });

  // Wrapper component that provides tRPC context
  const TrpcWrapper = ({ children }: { children: React.ReactNode }) => {
    const [queryClient] = React.useState(() => new QueryClient());

    return (
      <QueryClientProvider client={queryClient}>
        <trpc.Provider client={trpcClient} queryClient={queryClient}>
          {children}
        </trpc.Provider>
      </QueryClientProvider>
    );
  };

  it('should render login form', () => {
    render(<LoginDialog />, { wrapper: TrpcWrapper });

    expect(screen.getByText('ERP Login')).toBeInTheDocument();
    expect(screen.getByRole('textbox', { name: /Usuário/i })).toBeInTheDocument();
    expect(screen.getByLabelText('Senha:')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: /Login/i })).toBeInTheDocument();
  });

  it('should successfully login with valid torres credentials', async () => {
    const user = userEvent.setup();
    render(<LoginDialog />, { wrapper: TrpcWrapper });

    // Fill in credentials
    const userInput = screen.getByRole('textbox', { name: /Usuário/i });
    const passwordInput = screen.getByLabelText('Senha:');
    const loginButton = screen.getByRole('button', { name: /Login/i });

    await user.type(userInput, 'torres');
    await user.type(passwordInput, '1234');
    await user.click(loginButton);

    // Wait for the API request and successful login
    await waitFor(
      () => {
        // Check if token was stored
        const token = localStorage.getItem('auth-token');
        expect(token).toBeTruthy();
        expect(token).toContain('eyJ'); // JWT starts with eyJ (base64 encoded header)
      },
      { timeout: 10000 }
    );

    // Check auth store was updated
    const authState = useAuthStore.getState();
    expect(authState.user).toBeDefined();
    expect(authState.user?.user).toBe('torres');
    expect(authState.user?.tipo).toBe('ADMINISTRADOR');
    expect(authState.user?.nome).toBe('RODRIGO TORRES');
    expect(authState.token).toBeTruthy();
  }, 15000);

  it('should display error with invalid credentials', { timeout: 30000 }, async () => {
    const user = userEvent.setup();
    render(<LoginDialog />, { wrapper: TrpcWrapper });

    const userInput = screen.getByRole('textbox', { name: /Usuário/i });
    const passwordInput = screen.getByLabelText('Senha:');
    const loginButton = screen.getByRole('button', { name: /Login/i });

    await user.type(userInput, 'nonexistent');
    await user.type(passwordInput, 'wrongpassword');
    await user.click(loginButton);

    // Wait for error message
    let lastError = null;
    await waitFor(
      () => {
        const errorMessage = screen.queryByText(/Login inválido/i);
        if (!errorMessage) {
          // Capture what we see instead
          const allText = screen.queryAllByText(/./);
          lastError = allText.map(el => el.textContent).join(' | ');
        }
        expect(errorMessage).toBeInTheDocument();
      },
      { timeout: 20000 }
    ).catch(err => {
      console.log('Page content when error test failed:', lastError);
      throw err;
    });

    // Auth store should remain empty
    const authState = useAuthStore.getState();
    expect(authState.user).toBeNull();
    expect(authState.token).toBeNull();
  }, 25000);

  it('should show validation error for empty fields', async () => {
    const user = userEvent.setup();
    render(<LoginDialog />, { wrapper: TrpcWrapper });

    const loginButton = screen.getByRole('button', { name: /Login/i });
    await user.click(loginButton);

    // Should show validation errors
    await waitFor(() => {
      expect(screen.getByText(/Usuário é obrigatório/i)).toBeInTheDocument();
      expect(screen.getByText(/Senha é obrigatória/i)).toBeInTheDocument();
    });
  });

  it('should remember last user', async () => {
    const user = userEvent.setup();

    // First login
    const { unmount: unmount1 } = render(<LoginDialog />, { wrapper: TrpcWrapper });
    const userInput = screen.getByRole('textbox', { name: /Usuário/i }) as HTMLInputElement;
    const passwordInput = screen.getByLabelText('Senha:') as HTMLInputElement;
    const loginButton = screen.getByRole('button', { name: /Login/i });

    await user.type(userInput, 'torres');
    await user.type(passwordInput, '1234');
    await user.click(loginButton);

    await waitFor(
      () => {
        expect(localStorage.getItem('lastUser')).toBe('torres');
      },
      { timeout: 10000 }
    );

    // Unmount component completely
    unmount1();

    // Remount and check if lastUser is remembered
    render(<LoginDialog />, { wrapper: TrpcWrapper });
    const newUserInput = screen.getByRole('textbox', { name: /Usuário/i }) as HTMLInputElement;
    expect(newUserInput.value).toBe('torres');
  }, 15000);

  it('should save staging preference', async () => {
    const user = userEvent.setup();
    render(<LoginDialog />, { wrapper: TrpcWrapper });

    const stagingCheckbox = screen.getByLabelText('Ambiente de teste') as HTMLInputElement;
    const userInput = screen.getByRole('textbox', { name: /Usuário/i });
    const passwordInput = screen.getByLabelText('Senha:');
    const loginButton = screen.getByRole('button', { name: /Login/i });

    await user.click(stagingCheckbox);
    expect(stagingCheckbox.checked).toBe(true);

    await user.type(userInput, 'torres');
    await user.type(passwordInput, '1234');
    await user.click(loginButton);

    await waitFor(
      () => {
        expect(localStorage.getItem('staging')).toBe('true');
      },
      { timeout: 10000 }
    );
  }, 15000);

  it('should disable login button while submitting', async () => {
    const user = userEvent.setup();
    render(<LoginDialog />, { wrapper: TrpcWrapper });

    const userInput = screen.getByRole('textbox', { name: /Usuário/i });
    const passwordInput = screen.getByLabelText('Senha:');
    const loginButton = screen.getByRole('button', { name: /Login/i });

    await user.type(userInput, 'torres');
    await user.type(passwordInput, '1234');

    await user.click(loginButton);

    // Button should be disabled during submission
    expect(loginButton).toBeDisabled();

    // Wait for login to complete
    await waitFor(
      () => {
        // Button should be re-enabled after success
        expect(localStorage.getItem('auth-token')).toBeTruthy();
      },
      { timeout: 10000 }
    );
  }, 15000);
});
