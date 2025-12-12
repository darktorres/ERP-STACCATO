using FluentValidation;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Shared.Validators;

/// <summary>
/// Validator for AuthorizationRequest
/// Matches TypeScript authorizationSchema Zod validation
/// </summary>
public class AuthorizationRequestValidator : AbstractValidator<AuthorizationRequest>
{
    public AuthorizationRequestValidator()
    {
        RuleFor(x => x.User)
            .NotEmpty().WithMessage("Usuário é obrigatório");

        RuleFor(x => x.SenhaUsoUnico)
            .NotEmpty().WithMessage("Senha de uso único é obrigatória")
            .Length(4).WithMessage("Senha de uso único deve ter 4 caracteres")
            .Matches(@"^\d{4}$").WithMessage("Senha de uso único deve conter apenas números");
    }
}
