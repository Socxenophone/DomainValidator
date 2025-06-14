#include <stddef.h>
#include <string.h> // For strlen

/**
 * Validates a domain name according to RFC 1034/1035 with additional constraints:
 * - Total length: 1-253 characters (excluding trailing dot, if any)
 * - Labels: 1-63 characters, separated by dots
 * - Label characters: a-z, A-Z, 0-9, hyphen (not at start/end)
 * - TLD (last label): At least 2 characters, only letters
 * * @param domain Null-terminated domain string to validate
 * @return 1 if valid, 0 if invalid
 */
int is_valid_domain(const char *domain) {
    if (domain == NULL || *domain == '\0') {
        return 0; // Null or empty domain is invalid
    }

    size_t total_length = 0;
    size_t current_label_length = 0;
    const char *p = domain;
    const char *last_label_start = domain; // To mark the beginning of the last label

    // Check for leading dot
    if (*p == '.') {
        return 0; 
    }

    while (*p != '\0') {
        total_length++;
        if (total_length > 253) {
            return 0; // Total length exceeds limit
        }

        if (*p == '.') {
            if (current_label_length == 0) {
                return 0; // Empty label (consecutive dots or leading dot - though leading dot checked above)
            }
            // Check for hyphen at the end of the previous label
            if (*(p - 1) == '-') {
                return 0; 
            }
            last_label_start = p + 1; // Mark the start of the next label
            current_label_length = 0; // Reset for new label
        } else {
            if (current_label_length == 0 && *p == '-') {
                return 0; // Hyphen at label start
            }
            
            // Character check
            char c = *p;
            int valid_char = (c >= 'a' && c <= 'z') ||
                             (c >= 'A' && c <= 'Z') ||
                             (c >= '0' && c <= '9') ||
                             (c == '-');
            if (!valid_char) {
                return 0; // Invalid character
            }

            current_label_length++;
            if (current_label_length > 63) {
                return 0; // Label length exceeds limit
            }
        }
        p++;
    }

    // After the loop, check the last label
    if (current_label_length == 0) {
        return 0; // Domain ends with a dot or is just a dot
    }

    // Check for hyphen at the end of the last label
    if (*(p - 1) == '-') {
        return 0; 
    }

    // TLD validation (last label)
    // Based on the constraints in the comment: at least 2 characters, only letters
    if (current_label_length < 2) {
        return 0; // TLD too short
    }

    // Ensure all characters in the TLD are letters
    for (const char *ptr = last_label_start; ptr < p; ptr++) {
        char c = *ptr;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return 0; // Non-letter in TLD
        }
    }

    return 1; // All checks passed, domain is valid
}
