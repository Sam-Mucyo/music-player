# Documentation Guide for Network Music Player

This guide explains how to write documentation for the Network Music Player project using Doxygen.

## Setting Up Your Environment

1. Install required tools:
   ```bash
   # macOS
   brew install doxygen graphviz
   
   # Ubuntu/Debian
   sudo apt-get install doxygen graphviz
   ```

2. Set up the pre-commit hook (optional but recommended):
   ```bash
   cp pre-commit-hook.sh .git/hooks/pre-commit
   chmod +x .git/hooks/pre-commit
   ```

## Writing Doxygen Comments

### For Classes

```cpp
/**
 * @class ClassName
 * @brief Brief description of the class
 *
 * Detailed description of the class that can span
 * multiple lines and provide more context.
 *
 * @author Your Name
 */
class ClassName {
    // ...
};
```

### For Methods/Functions

```cpp
/**
 * @brief Brief description of what the function does
 *
 * Detailed description of the function, including any
 * important information about how to use it.
 *
 * @param paramName Description of the parameter
 * @param anotherParam Description of another parameter
 * @return Description of the return value
 * @throws Description of exceptions that might be thrown
 * @see RelatedFunction() or RelatedClass
 */
ReturnType functionName(ParamType paramName, AnotherType anotherParam);
```

### For Member Variables

```cpp
int counter; ///< Brief description of the counter variable

/// Detailed description of this variable that can span
/// multiple lines if needed
double complexVariable;
```

### For Enums

```cpp
/**
 * @enum MessageType
 * @brief Types of messages that can be sent between client and server
 */
enum class MessageType {
    SONG_LIST_REQUEST, ///< Request for the list of available songs
    SONG_LIST_RESPONSE, ///< Response containing the list of available songs
    SONG_REQUEST,       ///< Request for a specific song
    SONG_DATA,          ///< Song data being sent
    ERROR               ///< Error message
};
```

## Documentation Workflow

1. **Write documentation as you code**:
   - Document all public interfaces thoroughly
   - Include parameter descriptions, return values, and exceptions
   - Add examples where appropriate

2. **Generate documentation locally** to preview:
   ```bash
   doxygen Doxyfile
   open docs/html/index.html
   ```

3. **Commit your changes**:
   - The pre-commit hook will check for undocumented public methods
   - CI/CD will automatically generate and publish documentation to GitHub Pages

4. **View the published documentation** at your GitHub Pages URL:
   ```
   https://[your-username].github.io/[repository-name]/
   ```

## Best Practices

1. **Be concise but complete** - Documentation should be thorough but not verbose
2. **Document why, not just what** - Explain the purpose, not just the mechanics
3. **Keep documentation close to code** - Update docs when you change code
4. **Use consistent style** - Follow the same format throughout the project
5. **Document edge cases and limitations** - Note any constraints or special cases

## Common Doxygen Commands

- `@brief` - Short description
- `@param` - Parameter description
- `@return` - Return value description
- `@throws` - Exception description
- `@see` - Reference to related entities
- `@note` - Special notes about usage
- `@warning` - Important warnings
- `@code` / `@endcode` - Code examples
- `@todo` - Future work items

## Getting Help

If you have questions about documentation, please reach out to the team lead or refer to the [Doxygen Manual](https://www.doxygen.nl/manual/).
