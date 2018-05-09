#ifndef STL_UTILS_H
#define STL_UTILS_H


//!
//! Check if a container contains an item.
//!
//! @param container
//!		The container.
//!
//! @param element
//!		The element.
//!
//! @return
//!		Returns true if the item is in the container, false otherwise.
//!
template< typename ContainerType, typename Type >
bool Contains(const ContainerType & container, const Type & element)
{
	return std::find(container.begin(), container.end(), element) != container.end();
}

//!
//! Remove all occurence of an item from a container.
//!
//! @param container
//!		The container.
//!
//! @param item
//!		The item to remove.
//!
template< typename ContainerType, typename Type >
void Remove(ContainerType & container, const Type & item)
{
	auto end = std::remove(container.begin(), container.end(), item);
	if (end != container.end())
	{
		container.erase(end, container.end());
	}
}

//!
//! Get the index of an element in a container.
//!
//! @param container
//!		The container.
//!
//! @param element
//!		The element.
//!
//! @return
//!		The 0 based index of the element in the list if found, -1 otherwise.
//!
template< typename ContainerType, typename Type >
int IndexOf(const ContainerType & container, const Type & element)
{
	auto iterator = Find(container, element);
	return iterator == container.end() ? -1 : int(std::distance(container.begin(), iterator));
}


#endif // STL_UTILS_H
