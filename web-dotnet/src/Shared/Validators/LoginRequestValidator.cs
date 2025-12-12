using FluentValidation;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Shared.Validators;

/// <summary>
/// Validator for LoginRequest
/// Matches TypeScript loginSchema Zod validation
/// </summary>
public class LoginRequestValidator : AbstractValidator<LoginRequest>
{
    public LoginRequestValidator()
    {
        RuleFor(x => x.User)
            .NotEmpty().WithMessage("Usuário é obrigatório")
            .MaximumLength(20).WithMessage("Usuário deve ter no máximo 20 caracteres");

        RuleFor(x => x.Password)
            .NotEmpty().WithMessage("Senha é obrigatória")
            .MinimumLength(4).WithMessage("Senha deve ter no mínimo 4 caracteres");
    }
}
