
## Adding New Tests

1. Create a new test file in the `test/` directory
2. Add the file to `test/CMakeLists.txt`
3. Add test group function to `test/main.c`
4. Implement test cases
5. Create any necessary mocks

## Best Practices

1. Test both success and failure cases
2. Mock external dependencies
3. Keep tests focused and independent
4. Clean up after each test
5. Use meaningful test names and descriptions
6. Test edge cases and boundary conditions
7. Avoid testing implementation details
8. Keep test code as simple as possible

## Troubleshooting

### Common Issues

1. **Tests not building:**
   - Verify all test files are listed in `test/CMakeLists.txt`
   - Check for missing includes or dependencies

2. **Segmentation faults:**
   - Check for null pointer access
   - Verify mock initialization
   - Ensure proper cleanup between tests

3. **Link errors:**
   - Verify all required libraries are linked in CMakeLists.txt
   - Check for missing function implementations

### Getting Help

If you encounter issues:
1. Check the test output with `ctest --output-on-failure`
2. Run tests directly with `./test/run_tests` for more detail
3. Use a debugger: `gdb ./test/run_tests`