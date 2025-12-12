using Xunit;
using FluentAssertions;
using FluentValidation;
using ERP.Staccato.Shared.Models;
using ERP.Staccato.Shared.Validators;

namespace ERP.Staccato.Backend.Tests.Validators;

public class LoginRequestValidatorTests
{
    private readonly LoginRequestValidator _validator;

    public LoginRequestValidatorTests()
    {
        _validator = new LoginRequestValidator();
    }

    [Fact]
    public void Validate_WithValidRequest_ShouldPass()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "testuser",
            Password = "password123"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeTrue();
        result.Errors.Should().BeEmpty();
    }

    [Fact]
    public void Validate_WithEmptyUser_ShouldFail()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "",
            Password = "password123"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().ContainSingle();
        result.Errors[0].PropertyName.Should().Be("User");
        result.Errors[0].ErrorMessage.Should().Contain("obrigatório");
    }

    [Fact]
    public void Validate_WithNullUser_ShouldFail()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = null!,
            Password = "password123"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
    }

    [Fact]
    public void Validate_WithUserExceedingMaxLength_ShouldFail()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = new string('a', 21),
            Password = "password123"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors[0].PropertyName.Should().Be("User");
        result.Errors[0].ErrorMessage.Should().Contain("20 caracteres");
    }

    [Fact]
    public void Validate_WithEmptyPassword_ShouldFail()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "testuser",
            Password = ""
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().NotBeEmpty();
        result.Errors.Should().Contain(e => e.PropertyName == "Password" && e.ErrorMessage.Contains("obrigatória"));
    }

    [Fact]
    public void Validate_WithShortPassword_ShouldFail()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "testuser",
            Password = "123"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors[0].PropertyName.Should().Be("Password");
        result.Errors[0].ErrorMessage.Should().Contain("4 caracteres");
    }

    [Fact]
    public void Validate_WithMultipleErrors_ShouldReturnAllErrors()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "",
            Password = "123"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().HaveCount(2);
    }
}

public class AuthorizationRequestValidatorTests
{
    private readonly AuthorizationRequestValidator _validator;

    public AuthorizationRequestValidatorTests()
    {
        _validator = new AuthorizationRequestValidator();
    }

    [Fact]
    public void Validate_WithValidRequest_ShouldPass()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "1234"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeTrue();
        result.Errors.Should().BeEmpty();
    }

    [Fact]
    public void Validate_WithEmptyUser_ShouldFail()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "",
            SenhaUsoUnico = "1234"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().ContainSingle(e => e.PropertyName == "User");
    }

    [Fact]
    public void Validate_WithEmptyPassword_ShouldFail()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = ""
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().Contain(e => e.PropertyName == "SenhaUsoUnico" && e.ErrorMessage.Contains("obrigatória"));
    }

    [Fact]
    public void Validate_WithWrongPasswordLength_ShouldFail()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "12345"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors[0].PropertyName.Should().Be("SenhaUsoUnico");
        result.Errors[0].ErrorMessage.Should().Contain("4 caracteres");
    }

    [Fact]
    public void Validate_WithNonNumericPassword_ShouldFail()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "abcd"
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors[0].PropertyName.Should().Be("SenhaUsoUnico");
        result.Errors[0].ErrorMessage.Should().Contain("números");
    }

    [Theory]
    [InlineData("0000")]
    [InlineData("1234")]
    [InlineData("9999")]
    public void Validate_WithValidNumericPassword_ShouldPass(string password)
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = password
        };

        // Act
        var result = _validator.Validate(request);

        // Assert
        result.IsValid.Should().BeTrue();
    }
}
