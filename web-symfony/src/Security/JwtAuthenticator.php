<?php

namespace App\Security;

use App\Service\AuthService;
use Symfony\Component\HttpFoundation\JsonResponse;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Security\Core\Authentication\Token\TokenInterface;
use Symfony\Component\Security\Core\Exception\AuthenticationException;
use Symfony\Component\Security\Http\Authenticator\AbstractAuthenticator;
use Symfony\Component\Security\Http\Authenticator\Passport\Badge\UserBadge;
use Symfony\Component\Security\Http\Authenticator\Passport\Passport;
use Symfony\Component\Security\Http\Authenticator\Passport\SelfValidatingPassport;

class JwtAuthenticator extends AbstractAuthenticator
{
    private AuthService $authService;

    public function __construct(AuthService $authService)
    {
        $this->authService = $authService;
    }

    public function supports(Request $request): ?bool
    {
        // Only support API routes
        return str_starts_with($request->getPathInfo(), '/api/');
    }

    public function authenticate(Request $request): Passport
    {
        // Extract token from Authorization header
        $authHeader = $request->headers->get('Authorization');

        if (!$authHeader || !str_starts_with($authHeader, 'Bearer ')) {
            throw new AuthenticationException('Missing or invalid Authorization header');
        }

        $token = substr($authHeader, 7); // Remove "Bearer " prefix

        try {
            // Validate token
            $parsedToken = $this->authService->validateToken($token);

            // Get user from token
            $user = $this->authService->getUserFromToken($parsedToken);

            if (!$user) {
                throw new AuthenticationException('User not found');
            }

            // Return self-validating passport (token is already validated)
            return new SelfValidatingPassport(
                new UserBadge($user->getIdUsuario(), fn () => $user)
            );
        } catch (\Exception $e) {
            throw new AuthenticationException('Invalid token: ' . $e->getMessage());
        }
    }

    public function onAuthenticationSuccess(Request $request, TokenInterface $token, string $firewallName): ?Response
    {
        // Let the request continue to the controller
        return null;
    }

    public function onAuthenticationFailure(Request $request, AuthenticationException $exception): ?Response
    {
        return new JsonResponse([
            'success' => false,
            'error' => 'Authentication failed: ' . $exception->getMessageKey(),
        ], Response::HTTP_UNAUTHORIZED);
    }
}
